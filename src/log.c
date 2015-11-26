#include "log.h"


void LOG(const char *format, ...){ 
  FILE *logfile = fopen(const_log_file_path, "a");
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
  fclose(logfile);
}

void LOG_start(char *path){
	const_log_file_path = path;
	FILE *logfile = fopen(const_log_file_path, "w");
	fclose(logfile);
}