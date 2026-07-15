#pragma once

#include <string>

#include "../uiwindow.h"

class ExpiredCertPromptScreen : public UIWindow {
public:
  ExpiredCertPromptScreen(Ui *);
  void initialize(unsigned int row, unsigned int col, int connid, const std::string& sitename,
                  int expdays);
  void redraw() override;
  bool keyPressed(unsigned int ch) override;
  std::string getLegendText() const override;
  std::string getInfoLabel() const override;
private:
  int connid;
  std::string sitename;
  int expdays;
};
