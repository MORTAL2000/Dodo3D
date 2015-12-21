#pragma once

#include <stdio.h>

#define DODO_LOG(format, ...) \
    printf("<Dodo Log> "); \
    printf( format, ##__VA_ARGS__ ); \
    printf("\n");

