#include "queuescreen.h"

#include "../ui.h"
#include "../menuselectadjustableline.h"
#include "../menuselectoptionelement.h"
#include "../resizableelement.h"
#include "../menuselectoptiontextbutton.h"
#include "../misc.h"

#include "../../core/tickpoke.h"
#include "../../globalcontext.h"
#include "../../engine.h"
#include "../../transferjob.h"
#include "../../transferstatus.h"

namespace {
enum KeyAction {
  KEYACTION_NEXT_SOURCE,
  KEYACTION_MOVE_DOWN,
  KEYACTION_MOVE_UP,
  KEYACTION_STOP_RELEASE,
  KEYACTION_STOP_FILE,
  KEYACTION_CONTINUE
};
}

QueueScreen::QueueScreen(Ui* ui) : UIWindow(ui, "QueueScreen"), table(*vv) {
  keybinds.addBind(10, KEYACTION_INFO, "Details");
  keybinds.addBind('\t', KEYACTION_NEXT_SOURCE, "Next source");
  keybinds.addBind('t', KEYACTION_ENTER, "Start batch");
  keybinds.addBind('T', KEYACTION_START_ALL, "Start all batches");
  keybinds.addBind(KEY_DC, KEYACTION_DELETE, "Remove");
  keybinds.addBind('c', KEYACTION_CONTINUE, "Continue");
  keybinds.addBind('q', KEYACTION_BACK_CANCEL, "Return");
  keybinds.addBind('+', KEYACTION_MOVE_UP, "Move up");
  keybinds.addBind('-', KEYACTION_MOVE_DOWN, "Move down");
  keybinds.addBind('s', KEYACTION_STOP_RELEASE, "Stop after release");
  keybinds.addBind('S', KEYACTION_STOP_FILE, "Stop after file");
  keybinds.addBind('C', KEYACTION_CLEAR, "Clear source");
}

QueueScreen::~QueueScreen() {
  global->getTickPoke()->stopPoke(this, 0);
}

void QueueScreen::initialize(unsigned int row, unsigned int col) {
  autoupdate = true;
  hascontents = false;
  currentviewspan = 0;
  ypos = 1;
  animtick = 0;
  delete_pending_id = 0;
  clear_pending = false;
  restore_id = 0;
  currentsource = 0;
  routes.clear();
  sourceitems.clear();
  global->getTickPoke()->stopPoke(this, 0);
  engine = global->getEngine();
  table.reset();
  table.enterFocusFrom(0);
  global->getTickPoke()->startPoke(this, "QueueScreen", 500, 0);
  init(row, col);
}

void QueueScreen::redraw() {
  vv->clear();
  rebuildRoutes();
  table.reset();
  if (routes.empty()) {
    hascontents = false;
    printSlider(vv, row, col - 1, 1, currentviewspan);
    return;
  }
  if (currentsource >= routes.size()) {
    currentsource = routes.size() - 1;
  }
  sourceitems.clear();
  const std::string& routekey = routes[currentsource];
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
    std::shared_ptr<QueuedItem> qi = *it;
    if (qi->getDisplayKey() == routekey) {
      sourceitems.push_back(qi);
    }
  }
  vv->putStr(0, 1, getSourceDescription(), false);
  unsigned int y = 1;
  unsigned int totallistsize = sourceitems.size() + 1;
  while (ypos > 1 && ypos >= totallistsize) {
    --ypos;
  }
  adaptViewSpan(currentviewspan, row - 1, ypos, totallistsize);

  if (!currentviewspan) {
    std::shared_ptr<MenuSelectAdjustableLine> msal = table.addAdjustableLine();
    std::shared_ptr<MenuSelectOptionTextButton> msotb;
    msotb = table.addTextButtonNoContent(y, 1, "statush", "STATUS");
    msotb->setSelectable(false);
    msal->addElement(msotb, 7, RESIZE_REMOVE);
    msotb = table.addTextButtonNoContent(y, 8, "fileh", "FILE");
    msotb->setSelectable(false);
    msal->addElement(msotb, 12, 1, RESIZE_CUTEND, true);
    y++;
  }
  unsigned int pos = 1;
  for (auto it = sourceitems.begin(); it != sourceitems.end() && y < row; ++it) {
    std::shared_ptr<QueuedItem> qi = *it;
    if (pos >= currentviewspan) {
      std::string status = getFileStatusLabel(qi);
      std::shared_ptr<MenuSelectAdjustableLine> msal = table.addAdjustableLine();
      std::shared_ptr<MenuSelectOptionTextButton> msotb;
      msotb = table.addTextButtonNoContent(y, 1, "status", status);
      msotb->setSelectable(false);
      msal->addElement(msotb, 7, RESIZE_REMOVE);
      msotb = table.addTextButtonNoContent(y, 8, "file", qi->fileName);
      msotb->setSelectable(true);
      msotb->setId(qi->id);
      msal->addElement(msotb, 12, 1, RESIZE_CUTEND, true);
      if (pos == ypos) {
        table.enterFocusFrom(2);
      }
      y++;
    }
    ++pos;
  }
  table.checkPointer();
  hascontents = table.linesSize() > 1;
  table.adjustLines(col - 3);
  std::shared_ptr<MenuSelectAdjustableLine> highlightline;
  for (unsigned int i = 0; i < table.size(); i++) {
    std::shared_ptr<ResizableElement> re = std::static_pointer_cast<ResizableElement>(table.getElement(i));
    bool highlight = hascontents && table.getSelectionPointer() == i;
    if (re->isVisible()) {
      vv->putStr(re->getRow(), re->getCol(), re->getLabelText(), highlight);
    }
    if (highlight && ui->getHighlightEntireLine()) {
      highlightline = table.getAdjustableLine(re);
    }
  }
  if (highlightline) {
    std::pair<unsigned int, unsigned int> minmaxcol = highlightline->getMinMaxCol();
    vv->highlightOn(highlightline->getRow(), minmaxcol.first, minmaxcol.second - minmaxcol.first + 1);
  }
  printSlider(vv, row, col - 1, totallistsize, currentviewspan);
}

bool QueueScreen::keyPressed(unsigned int ch) {
  int action = keybinds.getKeyAction(ch);
  switch (action) {
    case KEYACTION_BACK_CANCEL:
      ui->returnToLast();
      return true;
    case KEYACTION_NEXT_SOURCE:
      if (routes.size() > 1) {
        currentsource = (currentsource + 1) % routes.size();
        ypos = 1;
        currentviewspan = 0;
        ui->redraw();
      }
      return true;
    case KEYACTION_INFO:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getSelectedItem();
        if (qi) {
          if (qi->transferJobId) {
            ui->goTransferJobStatus(qi->transferJobId);
          }
          else {
            std::string src;
            if (qi->direction == QueuedItem::Direction::UPLOAD) {
              src = qi->localDstPath.toString();
            }
            else {
              src = qi->srcSite + ":" + qi->srcPath;
            }
            std::string dst;
            if (qi->direction == QueuedItem::Direction::DOWNLOAD) {
              dst = qi->localDstPath.toString();
            }
            else {
              dst = qi->dstSite + ":" + qi->dstPath;
            }
            ui->goInfo(qi->getDirectionLabel() + ": " + src + "/" + qi->fileName + "  ->  " + dst);
          }
        }
      }
      return true;
    case KEYACTION_ENTER:
      if (!sourceitems.empty()) {
        animtick = 2;
        std::shared_ptr<QueuedItem> rep = nullptr;
        for (auto it = sourceitems.begin(); it != sourceitems.end(); ++it) {
          if (!(*it)->transferJobId) {
            rep = *it;
            break;
          }
        }
        if (rep) {
          JobStartResult result = engine->startQueueBatch(rep);
          if (result) {
            ui->addTempLegendTransferJob(result.id);
          }
          else if (!result.error.empty()) {
            ui->goInfo("Failed to start transfer: " + result.error);
          }
        }
        ui->redraw();
      }
      return true;
    case KEYACTION_START_ALL:
      if (!sourceitems.empty()) {
        animtick = 2;
        JobStartResult result = engine->startAllQueuedBatches();
        if (result) {
          ui->addTempLegendTransferJob(result.id);
        }
        else if (!result.error.empty()) {
          ui->goInfo("Failed to start transfer: " + result.error);
        }
        ui->redraw();
      }
      return true;
    case KEYACTION_DELETE:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getSelectedItem();
        if (qi) {
          delete_pending_id = qi->id;
          ui->goConfirmation("Remove from queue?");
        }
      }
      return true;
    case KEYACTION_CLEAR:
      if (!sourceitems.empty()) {
        clear_pending = true;
        ui->goConfirmation("Clear active source from queue?");
      }
      return true;
    case KEYACTION_MOVE_UP:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getSelectedItem();
        if (qi && engine->moveQueueItemUp(qi->id)) {
          if (ypos > 1) --ypos;
          ui->redraw();
        }
      }
      return true;
    case KEYACTION_MOVE_DOWN:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getSelectedItem();
        if (qi && engine->moveQueueItemDown(qi->id)) {
          if (ypos < sourceitems.size()) ++ypos;
          ui->redraw();
        }
      }
      return true;
    case KEYACTION_STOP_RELEASE:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getStartedItem();
        if (qi && qi->transferJobId) {
          std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
          if (tj && !tj->isDone()) {
            tj->stopAfterRelease();
          }
        }
        ui->redraw();
      }
      return true;
    case KEYACTION_STOP_FILE:
      if (hascontents) {
        std::shared_ptr<QueuedItem> qi = getStartedItem();
        if (qi && qi->transferJobId) {
          std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
          if (tj && !tj->isDone()) {
            tj->stopAfterFile();
          }
        }
        ui->redraw();
      }
      return true;
    case KEYACTION_CONTINUE:
      if (hascontents) {
        for (auto it = sourceitems.begin(); it != sourceitems.end(); ++it) {
          std::shared_ptr<QueuedItem> qi = *it;
          if (qi->transferJobId) {
            std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
            if (tj && tj->isStopping()) {
              tj->clearStopFlags();
            }
          }
        }
        ui->redraw();
      }
      return true;
  }
  return false;
}

bool QueueScreen::keyUp() {
  if (hascontents && ypos > 1) {
    --ypos;
    table.goUp();
    ui->update();
    return true;
  }
  return false;
}

bool QueueScreen::keyDown() {
  if (hascontents && ypos < sourceitems.size()) {
    ++ypos;
    table.goDown();
    ui->update();
    return true;
  }
  return false;
}

void QueueScreen::command(const std::string& command, const std::string& arg) {
  if (command == "yes") {
    if (clear_pending) {
      clear_pending = false;
      std::list<unsigned int> abortjobs;
      for (auto it = sourceitems.begin(); it != sourceitems.end(); ++it) {
        std::shared_ptr<QueuedItem> qi = *it;
        if (qi->transferJobId) {
          bool found = false;
          for (unsigned int jobid : abortjobs) {
            if (jobid == qi->transferJobId) {
              found = true;
              break;
            }
          }
          if (!found) {
            abortjobs.push_back(qi->transferJobId);
          }
        }
        engine->removeFromQueue(qi->id);
      }
      for (unsigned int jobid : abortjobs) {
        std::shared_ptr<TransferJob> tj = engine->getTransferJob(jobid);
        if (tj && !tj->isDone()) {
          engine->abortTransferJob(tj);
        }
      }
      ypos = 1;
      currentviewspan = 0;
      ui->redraw();
    }
    else if (delete_pending_id) {
      engine->removeFromQueue(delete_pending_id);
      delete_pending_id = 0;
      if (ypos > sourceitems.size()) {
        ypos = sourceitems.size();
      }
      ui->redraw();
    }
  }
  else {
    delete_pending_id = 0;
    clear_pending = false;
  }
}

std::string QueueScreen::getInfoLabel() const {
  return "TRANSFER QUEUE";
}

std::string QueueScreen::getInfoText() const {
  return std::to_string(engine->getQueueSize()) + " items (" + std::to_string(engine->countStartedQueueItems()) + " transferring)";
}

void QueueScreen::tick(int) {
  animtick++;
  if (engine->countStartedQueueItems() > 0) {
    ui->redraw();
  }
  checkCompleted();
}

void QueueScreen::checkCompleted() {
  bool changed = false;
  std::list<unsigned int> toremove;
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
    std::shared_ptr<QueuedItem> qi = *it;
    if (qi->transferJobId) {
      std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
      if (!tj || tj->getStatus() == TRANSFERJOB_DONE) {
        toremove.push_back(qi->id);
        changed = true;
      }
    }
  }
  for (unsigned int id : toremove) {
    engine->removeFromQueue(id);
  }
  if (changed) {
    ui->redraw();
  }
}

std::string QueueScreen::getLegendText() const {
  return keybinds.getLegendSummary();
}

void QueueScreen::rebuildRoutes() {
  std::vector<std::string> newroutes;
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
    std::string routekey = (*it)->getDisplayKey();
    bool found = false;
    for (const std::string& existing : newroutes) {
      if (existing == routekey) {
        found = true;
        break;
      }
    }
    if (!found) {
      newroutes.push_back(routekey);
    }
  }
  routes = newroutes;
}

std::string QueueScreen::getSourceDescription() const {
  if (routes.empty() || currentsource >= routes.size()) {
    return "";
  }
  std::shared_ptr<QueuedItem> rep = nullptr;
  for (auto it = sourceitems.begin(); it != sourceitems.end(); ++it) {
    if ((*it)->getDisplayKey() == routes[currentsource]) {
      rep = *it;
      break;
    }
  }
  std::string desc = "source " + std::to_string(currentsource + 1) + "/" + std::to_string(routes.size());
  if (rep) {
    desc += "  -  " + rep->getDirectionLabel();
    if (rep->direction == QueuedItem::Direction::UPLOAD) {
      desc += " " + rep->localDstPath.toString() + " -> " + rep->dstSite;
    }
    else {
      desc += " " + rep->srcSite;
      if (rep->direction == QueuedItem::Direction::DOWNLOAD) {
        desc += " -> " + rep->localDstPath.toString();
      }
      else {
        desc += " -> " + rep->dstSite;
      }
    }
  }
  return desc;
}

std::string QueueScreen::getFileStatusLabel(const std::shared_ptr<QueuedItem>& qi) const {
  if (!qi->transferJobId) {
    return "wait";
  }
  std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
  if (!tj) {
    return "wait";
  }
  switch (tj->getStatus()) {
    case TRANSFERJOB_FAILED:
      return "fail";
    case TRANSFERJOB_ABORTED:
      return "abor";
    case TRANSFERJOB_DONE:
      return "done";
    default:
      break;
  }
  std::shared_ptr<TransferStatus> ts = tj->getFileTransferStatus(qi->fileName);
  if (!ts) {
    unsigned int dots = (animtick / 4) % 4;
    return std::string(dots, '.');
  }
  switch (ts->getState()) {
    case TRANSFERSTATUS_STATE_IN_PROGRESS: {
      std::string prefix = "dn";
      if (ts->getType() == TRANSFERSTATUS_TYPE_UPLOAD) {
        prefix = "up";
      }
      else if (ts->getType() == TRANSFERSTATUS_TYPE_FXP) {
        prefix = "fxp";
      }
      return prefix + std::to_string(ts->getProgress()) + "%";
    }
    case TRANSFERSTATUS_STATE_SUCCESSFUL:
    case TRANSFERSTATUS_STATE_DUPE:
      return "done";
    case TRANSFERSTATUS_STATE_FAILED:
    case TRANSFERSTATUS_STATE_TIMEOUT:
      return "fail";
    case TRANSFERSTATUS_STATE_ABORTED:
      return "abor";
  }
  return "lgn";
}

std::shared_ptr<QueuedItem> QueueScreen::getSelectedItem() const {
  std::shared_ptr<MenuSelectOptionTextButton> msotb =
      std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
  return engine->getQueuedItemById(msotb->getId());
}

std::shared_ptr<QueuedItem> QueueScreen::getStartedItem() const {
  for (auto it = sourceitems.begin(); it != sourceitems.end(); ++it) {
    if ((*it)->transferJobId) {
      return *it;
    }
  }
  return nullptr;
}
