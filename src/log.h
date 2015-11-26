#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h> 
#include <stdio.h>

char *const_log_file_path;

void LOG(const char *format, ...);
void LOG_start();

#endif