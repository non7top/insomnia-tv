// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include <unity.h>

extern int runConfigTests(void);
extern int runHalTests(void);

#ifdef INSOMNIATV_NATIVE
int main(void) {
  UNITY_BEGIN();
  runConfigTests();
  runHalTests();
  return UNITY_END();
}
#endif
