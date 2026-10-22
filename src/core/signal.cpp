#include "signal.h"

#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <pthread.h>
#include <unistd.h>

#include "workmanager.h"

namespace Core {

namespace {

struct Sig {
  int sig;
  WorkManager* wm;
  EventReceiver* er;

};

std::map<int, Sig> registeredsignals;
std::mutex registeredsignalslock;

int signalpipe[2] = {-1, -1};
pthread_t signalreaderthread;
bool readerstarted = false;

void* signalReaderThread(void*) {
  // Runs in a dedicated thread, so all work here (map lookups, mutexes,
  // semaphore posts) is done outside signal context. The signal handler
  // itself only performs an async-signal-safe write() to the pipe.
  while (true) {
    unsigned char sig;
    ssize_t len = read(signalpipe[0], &sig, 1);
    if (len < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    if (len == 0) {
      break;
    }
    WorkManager* wm = nullptr;
    EventReceiver* er = nullptr;
    {
      std::lock_guard<std::mutex> lock(registeredsignalslock);
      std::map<int, Sig>::const_iterator it = registeredsignals.find(sig);
      if (it == registeredsignals.end()) {
        continue;
      }
      wm = it->second.wm;
      er = it->second.er;
    }
    wm->dispatchSignal(er, sig, 0);
  }
  return nullptr;
}

} // namespace

void signalHandlerDispatch(int sig) {
  // Only async-signal-safe operations are allowed here.
  if (signalpipe[1] == -1) {
    return;
  }
  unsigned char c = static_cast<unsigned char>(sig);
  ssize_t written = write(signalpipe[1], &c, 1);
  (void) written;
}

void registerSignalDispatch(int sig, WorkManager* wm, EventReceiver* er) {
  {
    std::lock_guard<std::mutex> lock(registeredsignalslock);
    assert(registeredsignals.find(sig) == registeredsignals.end());
    Sig s;
    s.sig = sig;
    s.wm = wm;
    s.er = er;
    registeredsignals[sig] = s;
  }
  if (!readerstarted && pipe(signalpipe) == 0) {
    int flags = fcntl(signalpipe[1], F_GETFL, 0);
    fcntl(signalpipe[1], F_SETFL, flags | O_NONBLOCK);
    readerstarted = true;
    pthread_create(&signalreaderthread, nullptr, signalReaderThread, nullptr);
  }
  registerSignalHandler(sig, signalHandlerDispatch);
}

void registerSignalHandler(int sig, void (*sighandler)(int)) {
  struct sigaction sa;
  sa.sa_handler = sighandler;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);
  sigaction(sig, &sa, nullptr);
}

void blockAllRegisteredSignals() {
  sigset_t set;
  sigemptyset(&set);
  std::lock_guard<std::mutex> lock(registeredsignalslock);
  for (const std::pair<const int, Sig>& sig : registeredsignals) {
    sigaddset(&set, sig.first);
  }
  pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

} // namespace Core
