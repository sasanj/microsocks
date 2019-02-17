/*
 * daemonize.c
 * This example daemonizes a process, writes a few log messages,
 * sleeps 20 seconds and terminates afterwards.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio_ext.h> /* FSETLOCKING_BYCALLER */
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

FILE *log_stream = 0;
int pid_fd;
int my_vfprintf(FILE *file, const char *fmt, va_list argp) {
  static char my_buffer[2048];
  flockfile(file); // Using file locking system for accessing `my_buffer` also.
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
  }
  fflush(file);
  funlockfile(file);
  return i + j;
}
int my_fprintf(FILE *file, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  int i = my_vfprintf(file, fmt, argp);
  va_end(argp);
  return i;
}
void signal_handler(sig) int sig;
{
  switch (sig) {
  case SIGHUP:
    if (log_stream != NULL)
      my_fprintf(log_stream, "got SIGHUB!\n");
    break;
  case SIGTERM:
    lockf(pid_fd, F_ULOCK, 0);
    if (log_stream != NULL) {
      my_fprintf(log_stream, "Goning Down\n");
      fclose(log_stream);
    }
    exit(EXIT_SUCCESS);
    break;
  }
}
extern void dolog(const char *fmt, ...) {
  if (log_stream != NULL) {
    va_list argp;
    va_start(argp, fmt);
    my_vfprintf(log_stream, fmt, argp);
    va_end(argp);
  }
}
extern void daemonize(char *log_file, char *pid_file) {
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0) {
    perror("error can not daemonize.");
    exit(EXIT_FAILURE);
  }
  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0) {
    perror("error: setsid().");
    exit(EXIT_FAILURE);
  }

  /* Catch, ignore and handle signals */
  // TODO: Implement a working signal handler */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0) {
    perror("error: second fork()");
    exit(EXIT_FAILURE);
  }
  /* Success: Let the parent terminate */
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Set new file permissions */
  umask(027);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  chdir("/tmp");

  /* Close all open file descriptors */
  int x;
  for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    close(x);
  }
  pid_fd = open(pid_file, O_RDWR | O_CREAT, 0640);
  if (pid_fd < 0)
    exit(EXIT_FAILURE); /* can not open */
  if (lockf(pid_fd, F_TLOCK, 0) < 0)
    exit(EXIT_FAILURE); /* can not lock */
  /* first instance continues */
  char str[100];
  x = snprintf(str, sizeof(str), "%d\n", getpid());
  write(pid_fd, str, x); /* record pid to lockfile */
  if (log_file != NULL) {
    log_stream = fopen(log_file, "a");
    if (log_stream == NULL) {
      lockf(pid_fd, F_ULOCK, 0);
      exit(EXIT_FAILURE);
    }
    __fsetlocking(log_stream, FSETLOCKING_BYCALLER);
    my_fprintf(log_stream, "Hello World\n");
  }
}
