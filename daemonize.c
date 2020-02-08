/*
 * daemonize.c
 */

#include <fcntl.h> /* O_CREATE */
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>   /* perror() */
#include <sys/stat.h> /* umask() */
#include <unistd.h>   /* _SC_OPEN_MAX */

/* from logger.c */
bool open_log();
void dolog(const char *fmt, ...);
void close_log();

int pid_fd = -1;
extern void daemon_cleanup() {
  if (pid_fd >= 0) {
    lockf(pid_fd, F_ULOCK, 0);
  }
}
extern void daemonize(const char *pid_file) {
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if (pid < 0) {
    dolog("error can not daemonize.");
    exit(EXIT_FAILURE);
  }
  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid() < 0) {
    dolog("error: setsid().");
    exit(EXIT_FAILURE);
  }

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0) {
    dolog("error: second fork()");
    exit(EXIT_FAILURE);
  }
  /* Success: Let the parent terminate */
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Set new file permissions */
  umask(027);

  /* Change the working directory to the /tmp directory */
  /* or another appropriated directory */
  chdir("/tmp");
  /* Temporary closing the log file. */
  close_log();
  /* Close all open file descriptors */
  int x;
  for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    close(x);
  }
  open_log();
  pid_fd = open(pid_file, O_RDWR | O_CREAT, 0640);
  if (pid_fd < 0) {
    dolog("can not open pid file \"%s\"\n", pid_file);
    exit(EXIT_FAILURE); /* can not open */
  }
  if (lockf(pid_fd, F_TLOCK, 0) < 0) {
    dolog("can not lock pid file \"%s\"\n", pid_file);
    exit(EXIT_FAILURE); /* can not lock */
  }
  /* first instance continues */
  char str[100];
  x = snprintf(str, sizeof(str), "%d\n", getpid());
  write(pid_fd, str, x); /* record pid to lockfile */
  dolog("sucessfuly daemonized. pid[%d], pidfile=\"%s\"\n", getpid(), pid_file);
}
