#include <time.h>
#include "dtime.h"

#ifdef WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

void dx_sleep(double milliseconds) {
  #ifdef WIN32
      Sleep(milliseconds);
  #else
      if (milliseconds >= 1000)
        sleep(milliseconds / 1000);
  #endif
}