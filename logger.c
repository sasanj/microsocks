#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h> /* FSETLOCKING_BYCALLER */
#include <sys/timeb.h>
#include <time.h>

FILE *log_stream = 0;

int my_vfprintf(FILE *file, const char *fmt, va_list argp) {
  static char my_buffer[2048];
  int err = errno; /* we need to save `errno` before doing any io. thankfully it
                      is thread local.*/
  flockfile(
      file); /* Using file locking system for accessing `my_buffer` also. */
  struct timeb ti;
  ftime(&ti); /* it is going to be obsolete */
  struct tm *local = localtime(&ti.time);
  int i = snprintf(my_buffer, sizeof(my_buffer),
                   "%04d/%02d/%02d %02d:%02d:%02d.%03d ", local->tm_year + 1900,
                   local->tm_mon + 1, local->tm_mday, local->tm_hour,
                   local->tm_min, local->tm_sec, ti.millitm);
  int j = vsnprintf(&(my_buffer[i]), sizeof(my_buffer) - i, fmt, argp);
  va_end(argp);
  if (i > 0) {
    char *buffer = my_buffer;
    int len = i + j;
    while (*buffer != '\0' && --len >= 0)
      putc_unlocked(*buffer++, file);
    fflush(file);
  }
  funlockfile(file);
  errno = err;
  return i + j;
}
int my_fprintf(FILE *file, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  int i = my_vfprintf(file, fmt, argp);
  va_end(argp);
  return i;
}

extern void dolog(const char *fmt, ...) {
  if (log_stream != NULL) {
    va_list argp;
    va_start(argp, fmt);
    my_vfprintf(log_stream, fmt, argp);
    va_end(argp);
  }
}
extern bool prepare_log(const char *log_filename) {
  if (log_filename != NULL) {
    log_stream = fopen(log_filename, "a");
    if (log_stream == NULL) {
      return false;
    }

    __fsetlocking(log_stream, FSETLOCKING_BYCALLER);
  }
  return true;
}
extern void close_log() {
  fclose(log_stream);
  log_stream = 0;
}
