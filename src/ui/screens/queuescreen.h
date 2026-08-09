#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "../../core/eventreceiver.h"

#include "../uiwindow.h"
#include "../menuselectoption.h"

class Engine;
class QueuedItem;

class QueueScreen : public UIWindow, protected Core::EventReceiver {
public:
  QueueScreen(Ui *);
  ~QueueScreen();
  void initialize(unsigned int row, unsigned int col);
  void redraw() override;
  bool keyPressed(unsigned int) override;
  void command(const std::string& command, const std::string& arg) override;
  std::string getInfoLabel() const override;
  std::string getInfoText() const override;
  std::string getLegendText() const override;
private:
  bool keyUp() override;
  bool keyDown() override;
  void tick(int) override;
  void checkCompleted();
  void rebuildRoutes();
  std::string getSourceDescription() const;
  std::string getFileStatusLabel(const std::shared_ptr<QueuedItem>& qi) const;
  std::shared_ptr<QueuedItem> getSelectedItem() const;
  std::shared_ptr<QueuedItem> getStartedItem() const;
  unsigned int animtick;
  unsigned int delete_pending_id;
  bool clear_pending;
  unsigned int restore_id;
  MenuSelectOption table;
  Engine * engine;
  bool hascontents;
  unsigned int currentviewspan;
  unsigned int ypos;
  std::vector<std::string> routes;
  unsigned int currentsource;
  std::list<std::shared_ptr<QueuedItem>> sourceitems;
};
