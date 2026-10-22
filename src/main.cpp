#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <sys/resource.h>

#include "core/workmanager.h"
#include "core/iomanager.h"
#include "core/tickpoke.h"
#include "core/threading.h"
#include "core/logger.h"
#include "core/signal.h"
#include "settingsloadersaver.h"
#include "sitelogicmanager.h"
#include "transfermanager.h"
#include "sitemanager.h"
#include "globalcontext.h"
#include "engine.h"
#include "remotecommandhandler.h"
#include "skiplist.h"
#include "eventlog.h"
#include "proxymanager.h"
#include "localstorage.h"
#include "timereference.h"
#include "uibase.h"
#include "statistics.h"
#include "sectionmanager.h"
#include "httpserver.h"
#include "restapi.h"
#include "loadmonitor.h"
#include "externalscriptsmanager.h"
#include "subprocessmanager.h"
#include "logmanager.h"

namespace {

/* Receives termination signals on the WorkManager thread (via the signal
 * self-pipe) so that breakLoop() is never called from signal context. */
class Terminator : public Core::EventReceiver {
public:
  void signal(int sig, int value) override {
    global->getTickPoke()->breakLoop();
  }
};

class Main {
public:
  Main() {
    TimeReference::updateTime();

    Core::WorkManager* wm = new Core::WorkManager();
    Core::TickPoke* tp = new Core::TickPoke(*wm);
    Core::IOManager* iom = new Core::IOManager(*wm, *tp);

    global->linkCore(wm, tp, iom);

    TimeReference* tr = new TimeReference();
    global->linkTimeReference(tr);

    std::shared_ptr<EventLog> el = std::make_shared<EventLog>();
    Core::setLogger(el);
    global->linkEventLogger(el);

    SettingsLoaderSaver* sls = new SettingsLoaderSaver();
    LocalStorage* ls = new LocalStorage();
    Engine* e = new Engine();
    SiteManager* sm = new SiteManager();
    SiteLogicManager* slm = new SiteLogicManager();
    TransferManager* tm = new TransferManager();
    RemoteCommandHandler* rch = new RemoteCommandHandler();
    SkipList* sl = new SkipList();
    ProxyManager* pm = new ProxyManager();
    Statistics* s = new Statistics();
    SectionManager* secm = new SectionManager();
    HTTPServer* httprv = new HTTPServer();
    RestApi* ra = new RestApi();
    LoadMonitor* lm = new LoadMonitor();
    ExternalScriptsManager* esm = new ExternalScriptsManager();
    SubProcessManager* spm = new SubProcessManager();
    LogManager* logm = new LogManager();

    UIBase* uibase = UIBase::instance();

    global->linkComponents(sls, e, uibase, sm, slm, tm, rch, sl, pm, ls,
                           s, secm, httprv, ra, lm, esm, spm, logm);

    Core::Threading::setCurrentThreadName("cbftp");

    if (!uibase->init()) exit(1);
    wm->init("cbftp");
    iom->init("cbftp");
    Core::registerSignalDispatch(SIGABRT, wm, &terminator);
    Core::registerSignalDispatch(SIGTERM, wm, &terminator);
    Core::registerSignalDispatch(SIGINT, wm, &terminator);
    Core::registerSignalDispatch(SIGQUIT, wm, &terminator);
    sls->init();
    tp->tickerLoop();
    global->getSubProcessManager()->killAll();
    uibase->kill();
    sls->saveSettings();
  }
private:
  Terminator terminator;
};

} // namespace

int main(int argc, char* argv[]) {
  struct rlimit rl = {0, 0};
  setrlimit(RLIMIT_CORE, &rl);

  Main();
}


