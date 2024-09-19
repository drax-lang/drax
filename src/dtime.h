#ifndef __DTIME
#define __DTIME

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep(1000 * (x))
#else
    #include <time.h>
#endif

void dx_sleep(double milliseconds);

#endif