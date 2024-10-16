#ifndef MULTIPROCESS_UTIL
#define MULTIPROCESS_UTIL
#include <stdio.h>
#include <stdlib.h>
void true_exit(bool flag, const char* s)
{
    if(flag == true)
    {
        perror(s);
        exit(0);
    }
}

void true_show(bool flag, const char* s)
{
    if(flag == true)
    {
        perror(s);
    }
}
#endif
