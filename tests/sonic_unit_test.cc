#include "testing/base/public/gunit.h"
#include "third_party/sonic/tests/tests.h"

namespace {

TEST(SonicUnitTests, ClampTest) {
  EXPECT_TRUE(sonicTestInputClamping());
  EXPECT_TRUE(sonicTestInputsDontCrash());
}

}  // namespace
