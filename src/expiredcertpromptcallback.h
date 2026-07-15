#pragma once

#include <string>

class ExpiredCertPromptCallback {
public:
  virtual ~ExpiredCertPromptCallback() = default;

  virtual void expiredCertPromptRequired(void* logic,
                                         int connid,
                                         const std::string& sitename,
                                         int expdays) = 0;
};
