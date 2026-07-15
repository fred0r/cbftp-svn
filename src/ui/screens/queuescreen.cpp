#include "queuescreen.h"

#include "../ui.h"
#include "../menuselectadjustableline.h"
#include "../menuselectoptionelement.h"
#include "../resizableelement.h"
#include "../menuselectoptionalttextbutton.h"
#include "../menuselectoptiontextbutton.h"
#include "../misc.h"

#include "../../core/tickpoke.h"
#include "../../globalcontext.h"
#include "../../util.h"
#include "../../engine.h"
#include "../../transferjob.h"

namespace {
enum KeyAction {
  KEYACTION_MOVE_DOWN,
  KEYACTION_MOVE_UP,
  KEYACTION_STOP_RELEASE,
  KEYACTION_STOP_FILE,
  KEYACTION_CONTINUE
};
}

QueueScreen::QueueScreen(Ui* ui) : UIWindow(ui, "QueueScreen"), table(*vv) {
  keybinds.addBind(10, KEYACTION_INFO, "Details");
  keybinds.addBind('t', KEYACTION_ENTER, "Start selected");
  keybinds.addBind('T', KEYACTION_START_ALL, "Start all");
  keybinds.addBind(KEY_DC, KEYACTION_DELETE, "Remove");
  keybinds.addBind('c', KEYACTION_CONTINUE, "Continue");
  keybinds.addBind('q', KEYACTION_BACK_CANCEL, "Return");
  keybinds.addBind('+', KEYACTION_MOVE_UP, "Move up");
  keybinds.addBind('-', KEYACTION_MOVE_DOWN, "Move down");
  keybinds.addBind('s', KEYACTION_STOP_RELEASE, "Stop after release");
  keybinds.addBind('S', KEYACTION_STOP_FILE, "Stop after file");
  keybinds.addBind('C', KEYACTION_CLEAR, "Clear all");
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
  queueAutoChain = false;
  engine = global->getEngine();
  table.reset();
  table.enterFocusFrom(0);
  global->getTickPoke()->startPoke(this, "QueueScreen", 500, 0);
  init(row, col);
}

void QueueScreen::redraw() {
  vv->clear();
  unsigned int y = 0;
  unsigned int totallistsize = engine->getQueueSize() + 1;
  table.reset();
  while (ypos > 1 && ypos >= totallistsize) {
    --ypos;
  }
  adaptViewSpan(currentviewspan, row, ypos, totallistsize);

  if (!currentviewspan) {
    std::shared_ptr<MenuSelectAdjustableLine> msal = table.addAdjustableLine();
    std::shared_ptr<MenuSelectOptionTextButton> msotb;
    msotb = table.addTextButtonNoContent(y, 1, "typeh", "TYPE");
    msotb->setSelectable(false);
    msal->addElement(msotb, 6, RESIZE_REMOVE);
    msotb = table.addTextButtonNoContent(y, 1, "fromh", "FROM");
    msotb->setSelectable(false);
    msal->addElement(msotb, 10, RESIZE_REMOVE);
    msotb = table.addTextButtonNoContent(y, 1, "fileh", "FILE");
    msotb->setSelectable(false);
    msal->addElement(msotb, 12, 1, RESIZE_CUTEND, true);
    msotb = table.addTextButtonNoContent(y, 1, "toh", "TO");
    msotb->setSelectable(false);
    msal->addElement(msotb, 10, RESIZE_REMOVE);
    y++;
  }
  unsigned int pos = 1;
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd() && y < row; ++it) {
    if (pos >= currentviewspan) {
      std::shared_ptr<QueuedItem> qi = *it;
      std::string type;
      std::string dir = qi->getDirectionLabel();
      if (qi->transferJobId) {
        std::shared_ptr<TransferJob> tj = engine->getTransferJob(qi->transferJobId);
        if (tj) {
          switch (tj->getStatus()) {
            case TRANSFERJOB_RUNNING: {
              if (tj->isStopping()) {
                if (tj->isStopAfterFile()) {
                  type = "S" + dir;
                } else {
                  type = "s" + dir;
                }
              }
              else if (tj->getSpeed() > 0) {
                unsigned int dots = (animtick / 4) % 4;
                type = dir + std::string(dots, '.');
              }
              else {
                type = dir;
              }
              break;
            }
            case TRANSFERJOB_FAILED:
            case TRANSFERJOB_ABORTED:
              type = "x" + dir;
              break;
            default:
              type = dir;
              break;
          }
        }
        else {
          type = "x" + dir;
        }
      }
      else {
        type = dir;
      }
      while (type.length() < 6) {
        type += ' ';
      }
      std::string from = qi->getDisplaySource();
      std::string to = qi->getDisplayDest();

      std::shared_ptr<MenuSelectAdjustableLine> msal = table.addAdjustableLine();
      std::shared_ptr<MenuSelectOptionTextButton> msotb;
      msotb = table.addTextButtonNoContent(y, 1, "type", type);
      msotb->setSelectable(false);
      msal->addElement(msotb, 6, RESIZE_REMOVE);
      msotb = table.addTextButtonNoContent(y, 1, "from", from);
      msotb->setSelectable(false);
      msal->addElement(msotb, 10, RESIZE_REMOVE);
      msotb = table.addTextButtonNoContent(y, 1, "file", qi->fileName);
      msotb->setSelectable(true);
      msotb->setId(qi->id);
      msal->addElement(msotb, 12, 1, RESIZE_CUTEND, true);
      msotb = table.addTextButtonNoContent(y, 1, "to", to);
      msotb->setSelectable(false);
      msal->addElement(msotb, 10, RESIZE_REMOVE);
      y++;

      if (pos == ypos) {
        table.enterFocusFrom(2);
      }
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
    case KEYACTION_INFO:
      if (hascontents) {
        std::shared_ptr<MenuSelectOptionTextButton> msotb =
            std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
        unsigned int id = msotb->getId();
        std::shared_ptr<QueuedItem> qi = engine->getQueuedItemById(id);
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
      if (hascontents) {
        queueAutoChain = false;
        for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
          std::shared_ptr<QueuedItem> qi = *it;
          if (!qi->transferJobId) {
            animtick = 2;
            JobStartResult result = engine->startQueuedItem(qi);
            if (result) {
              ui->addTempLegendTransferJob(result.id);
              ui->redraw();
            }
            else {
              ui->goInfo("Failed to start transfer: " + result.error);
            }
            break;
          }
        }
      }
      return true;
    case KEYACTION_DELETE:
      if (hascontents) {
        std::shared_ptr<MenuSelectOptionTextButton> msotb =
            std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
        delete_pending_id = msotb->getId();
        ui->goConfirmation("Remove from queue?");
      }
      return true;
    case KEYACTION_CLEAR:
      clear_pending = true;
      ui->goConfirmation("Clear entire queue?");
      return true;
    case KEYACTION_START_ALL:
    {
      animtick = 2;
      queueAutoChain = true;
      for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
        std::shared_ptr<QueuedItem> qi = *it;
        if (qi->transferJobId) continue;
        JobStartResult result = engine->startQueuedItem(qi);
        if (result) {
          ui->addTempLegendTransferJob(result.id);
        }
        break;
      }
      ui->redraw();
      ui->setInfo();
      return true;
    }
    case KEYACTION_MOVE_UP:
      if (hascontents) {
        std::shared_ptr<MenuSelectOptionTextButton> msotb =
            std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
        unsigned int id = msotb->getId();
        if (engine->moveQueueItemUp(id)) {
          if (ypos > 1) --ypos;
          ui->redraw();
        }
      }
      return true;
    case KEYACTION_MOVE_DOWN:
      if (hascontents) {
        std::shared_ptr<MenuSelectOptionTextButton> msotb =
            std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
        unsigned int id = msotb->getId();
        if (engine->moveQueueItemDown(id)) {
          if (ypos < engine->getQueueSize()) ++ypos;
          ui->redraw();
        }
      }
      return true;
    case KEYACTION_STOP_RELEASE:
      if (hascontents) {
        engine->stopTopOfQueue(true);
        ui->redraw();
      }
      return true;
    case KEYACTION_STOP_FILE:
      if (hascontents) {
        engine->stopTopOfQueue(false);
        ui->redraw();
      }
      return true;
    case KEYACTION_CONTINUE:
      if (hascontents) {
        for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
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
  if (hascontents && ypos < engine->getQueueSize()) {
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
      engine->clearQueue();
      clear_pending = false;
      ypos = 1;
      currentviewspan = 0;
      ui->redraw();
    }
    else if (delete_pending_id) {
      engine->removeFromQueue(delete_pending_id);
      delete_pending_id = 0;
      if (ypos > engine->getQueueSize()) {
        ypos = engine->getQueueSize();
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
  unsigned int transferring = 0;
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
    if ((*it)->transferJobId) transferring++;
  }
  return std::to_string(engine->getQueueSize()) + " items (" + std::to_string(transferring) + " transferring)";
}

void QueueScreen::tick(int) {
  animtick++;
  bool hasrunning = false;
  for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
    if ((*it)->transferJobId) {
      hasrunning = true;
      break;
    }
  }
  if (hasrunning) {
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
  if (changed && queueAutoChain) {
    for (auto it = engine->getQueueBegin(); it != engine->getQueueEnd(); ++it) {
      std::shared_ptr<QueuedItem> qi = *it;
      if (!qi->transferJobId) {
        JobStartResult result = engine->startQueuedItem(qi);
        if (result) {
          ui->addTempLegendTransferJob(result.id);
        }
        break;
      }
    }
    ui->redraw();
  }
}

std::string QueueScreen::getLegendText() const {
  return keybinds.getLegendSummary();
}
