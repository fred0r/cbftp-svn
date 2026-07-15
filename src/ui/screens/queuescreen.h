#pragma once

#include <memory>

#include "../../core/eventreceiver.h"

#include "../uiwindow.h"
#include "../menuselectoption.h"

class Engine;

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
  unsigned int animtick;
  unsigned int delete_pending_id;
  bool clear_pending;
  unsigned int restore_id;
  MenuSelectOption table;
  Engine * engine;
  bool hascontents;
  unsigned int currentviewspan;
  unsigned int ypos;
  bool queueAutoChain;
};

