#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h> 
#include <stdio.h>

char *const_log_file_path;
char *dns_log;

void LOG(const char *format, ...);
void LOG_start();


void TEST_LOG(const char *format, ...);
void TEST_LOG_start(char *path);

void N_LOG(const char *format, ...);
void N_LOG_start(char *path);
#endif
