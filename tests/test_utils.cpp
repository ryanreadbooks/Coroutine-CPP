#include "ahri/utils.h"
#include <assert.h>
void test_backtrace() {
  printf("%s\n", ahri::GetBackTraceStr().c_str());
}

void test_assert() {
  // AHRI_ASSERT(0 == 1);
  AHRI_ASSERT_MSG(1 == 1, "0 IS NOT EQUAL TO 1");
}

int main(int argc, char** argv) {

  test_backtrace();
  test_assert();
  return 0;
}