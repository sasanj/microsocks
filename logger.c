#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h> /* FSETLOCKING_BYCALLER */
#include <stdlib.h>    /* free() */
#include <string.h>    /* strdup() */
#include <sys/timeb.h>
#include <time.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
char *log_file = NULL;
FILE *log_stream = 0;
/**
 * prints a log with time at first.
 * IT IS NOT THREAD SAFE.
 * */
int my_vfprintf(FILE *file, const char *fmt, va_list argp) {
  static char my_buffer[2048];
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
    int err = errno; /* we need to save `errno` before doing any io. thankfully
                      it is thread local.*/
    pthread_mutex_lock(&mutex1);
    my_vfprintf(log_stream, fmt, argp);
    pthread_mutex_unlock(&mutex1);
    errno = err;
    va_end(argp);
  }
}
extern void close_log() {
  if (log_stream == 0)
    return;
  pthread_mutex_lock(&mutex1);
  fclose(log_stream);
  log_stream = 0;
  pthread_mutex_unlock(&mutex1);
}
extern bool set_log_file_name(const char *log_filename) {
  if (log_stream != 0) {
    /* there is a open stream on current file name. */
    return false;
  }
  pthread_mutex_lock(&mutex1);
  if (log_file != NULL) {
    free(log_file);
    log_file = 0;
  }
  if (log_filename != NULL)
    log_file = strdup(log_filename);
  pthread_mutex_unlock(&mutex1);
  return true;
}
extern bool open_log() {
  if (log_stream != 0) {
    /* there is a stream open. */
    return false;
  }
  if (log_file == NULL) {
    /* no log file name! */
    return false;
  }
  pthread_mutex_lock(&mutex1);
  log_stream = fopen(log_file, "a");
  if (log_stream == NULL) {
    pthread_mutex_unlock(&mutex1);
    return false;
  }

  __fsetlocking(log_stream, FSETLOCKING_BYCALLER);
  pthread_mutex_unlock(&mutex1);
  return true;
}
extern bool prepare_log(const char *log_filename) {
  set_log_file_name(log_filename);
  return open_log();
}
