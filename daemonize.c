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

bool prepare_log(const char *log_filename);
void dolog(const char *fmt, ...);
void close_log();

int pid_fd;
void signal_handler(sig) int sig;
{
  switch (sig) {
  case SIGHUP:
    /* TODO: add log rotation. */
    dolog("got SIGHUB!\n");
    break;
  case SIGTERM:
    dolog("going down.");
    close_log();
    lockf(pid_fd, F_ULOCK, 0);
    exit(EXIT_SUCCESS);
    break;
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

  /* Change the working directory to the /tmp directory */
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
  if (prepare_log(log_file))
    dolog("microproxy started\n");
  else {
    lockf(pid_fd, F_ULOCK, 0);
    exit(EXIT_FAILURE);
  }
}
