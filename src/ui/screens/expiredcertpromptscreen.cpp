#include "expiredcertpromptscreen.h"

#include "../ui.h"

ExpiredCertPromptScreen::ExpiredCertPromptScreen(Ui* ui) : UIWindow(ui, "ExpiredCertPromptScreen") {
  allowimplicitgokeybinds = false;
}

void ExpiredCertPromptScreen::initialize(unsigned int row, unsigned int col,
                                               int connid,
                                               const std::string& sitename,
                                               int expdays) {
  this->connid = connid;
  this->sitename = sitename;
  this->expdays = expdays;
  init(row, col);
}

void ExpiredCertPromptScreen::redraw() {
  vv->clear();
  vv->putStr(1, 1, "WARNING: TLS certificate has EXPIRED for site: " + sitename);
  if (expdays > 0) {
    vv->putStr(3, 1, "Certificate expired " + std::to_string(expdays) + " days ago");
  }
  vv->putStr(5, 1, "Connecting anyway may be unsafe.");
  vv->putStr(7, 1, "Do you want to connect?");
  vv->putStr(9, 1, "[y] Yes, connect anyway");
  vv->putStr(10, 1, "[n] No, disconnect");
}

bool ExpiredCertPromptScreen::keyPressed(unsigned int ch) {
  if (ch == 'y' || ch == 'Y') {
    ui->expiredCertPromptYes(connid);
    return true;
  }
  else if (ch == 'n' || ch == 'N') {
    ui->expiredCertPromptNo(connid);
    return true;
  }
  return false;
}

std::string ExpiredCertPromptScreen::getLegendText() const {
  return "[y]es connect anyway - [n]o disconnect";
}

std::string ExpiredCertPromptScreen::getInfoLabel() const {
  return "EXPIRED TLS CERTIFICATE";
}
