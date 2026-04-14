#ifndef LOGGER_H
#define LOGGER_H

typedef void (*LogCallback)(const char* message, int level);

extern LogCallback g_logger;

void set_logger_callback(LogCallback cb);
void log_msg(int level, const char* format, ...);

#endif