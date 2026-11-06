#include "unit_test_support.h"
#include "thread_example.h"
#include "skeleton_exception.h"

using namespace Skeleton;

BOOST_FIXTURE_TEST_SUITE(thread_example, GlobalFixture)

namespace {
class Throws : public DirectorExample {
public:
  Throws() : DirectorExample(0, "throws") {}
  int func(int v) const override
  {
    if(v == 2)
      throw Exception("func failed for {}", v);
    return v;
  }
};
}

BOOST_AUTO_TEST_CASE(run)
{
  auto d = boost::make_shared<DirectorExample>(1, "threads");
  // (1+1) + (2+1) + (3+1) + (4+1)
  BOOST_CHECK_EQUAL(ThreadExample::run(d, 1, 4), 14);
  auto t = boost::make_shared<Throws>();
  BOOST_CHECK_EQUAL(ThreadExample::run(t, 0, 2), 1);
  BOOST_CHECK_EXCEPTION(ThreadExample::run(t, 0, 4), Exception,
                        [](const Exception& e) {
                          return std::string(e.what()) == "func failed for 2";
                        });
}

BOOST_AUTO_TEST_SUITE_END()
