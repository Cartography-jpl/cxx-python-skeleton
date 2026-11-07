#pragma once
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  This is a global fixture that is available to all unit tests.

  Thread safety: for the unit tests (the constructor sets an
  environment variable).
*******************************************************************/
class GlobalFixture {
public:
  GlobalFixture();
  [[nodiscard]] std::string test_data_dir() const;
private:
  void set_default_value();
};
}
