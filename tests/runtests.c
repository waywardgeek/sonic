/* Sonic library
   Copyright 2020
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

/* Run unit tests. */

#include "tests.h"

#include <stdio.h>

/* Unlike assert(), stays active under -DNDEBUG. */
#define CHECK(test)                              \
  do {                                           \
    if (!(test)) {                               \
      fprintf(stderr, "FAILED: %s\n", #test);    \
      return 1;                                  \
    }                                            \
  } while (0)

int main(int argc, char** argv) {
  CHECK(sonicTestInputClamping());
  CHECK(sonicTestInputsDontCrash());
  CHECK(sonicTestStreamCreation());
  CHECK(sonicTestParameters());
  CHECK(sonicTestFlush());
  CHECK(sonicTestSimpleProcessing());
  printf("All tests passed.\n");
  return 0;
}
