#pragma once
#include <string>

namespace Skeleton {
/****************************************************************/ /**
  This is a global fixture that is available to all unit tests.
*******************************************************************/
class GlobalFixture {
public:
  GlobalFixture();
  virtual ~GlobalFixture() = default;
  std::string test_data_dir() const;
private:
  void set_default_value();
};
}
