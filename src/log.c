#include "log.h"

int log_start = 0;

void LOG(const char *format, ...){ 
  if( log_start == 0) LOG_start();
  FILE *logfile = fopen("/tmp/log", "a");
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  fclose(logfile);
}

void LOG_start(char *path){
	if( log_start == 1) return;
	FILE *logfile = fopen("/tmp/log", "w");
	fclose(logfile);
	log_start = 1;
}

void TEST_LOG(const char *format, ...){ 
  FILE *logfile = fopen(const_log_file_path, "a");
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  fclose(logfile);
}

void TEST_LOG_start(char *path){
	const_log_file_path = path;
	FILE *logfile = fopen(const_log_file_path, "w");
	fclose(logfile);
}

void N_LOG(const char *format, ...){ 
  FILE *logfile = fopen(const_log_file_path, "a");
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  fclose(logfile);
}

void N_LOG_start(char *path){
	const_log_file_path = path;
	FILE *logfile = fopen(const_log_file_path, "w");
	fclose(logfile);
}
