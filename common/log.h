#pragma once

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_MODE 1

#if DEBUG_MODE

#define LOG(type, fmt, args...) \
    do {\
        printf("%s: %s@%s, line: %d\n***log_info[", type, __FILE__, __FUNCTION__, __LINE__);\
        printf(fmt, ##args);\
        printf("]\n\n");\
    } while (0)
#define DEBUG(fmt, args...) LOG("DEBUG", fmt, ##args)
#define ERROR(fmt, args...) do{LOG("ERROR", fmt, ##args);exit(0);}while(0)

#else

#define LOG(fmt, args...)
#define DEBUG(fmt, args...)
#define ERROR(fmt, args...)

#endif