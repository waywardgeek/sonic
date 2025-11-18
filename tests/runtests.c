/* Sonic library
   Copyright 2020
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

/* Run unit tests. */

#include "tests.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char** argv) {
  assert(sonicTestInputClamping());
  assert(sonicTestInputsDontCrash());
  assert(sonicTestStreamCreation());
  assert(sonicTestParameters());
  assert(sonicTestFlush());
  assert(sonicTestSimpleProcessing());
  printf("All tests passed.\n");
  return 0;
}
