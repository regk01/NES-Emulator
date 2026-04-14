#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

// Global Logger
LogCallback g_logger = NULL;

void set_logger_callback(LogCallback cb) {
    g_logger = cb;
}

void log_msg(int level, const char* format, ...) {
    if (!g_logger) return;

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    g_logger(buffer, level);
}