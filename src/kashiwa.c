/*
** kashiwa.c - kashiwa's main function
**
** Copyright (c) Uchio Kondo
**
** See Copyright Notice in LICENSE
*/

#define _GNU_SOURCE

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/random.h>

#include "kashiwa.h"

struct kash_handler {
  char *mruby_script_path;
  char *mruby_code;
};

static int kash_do_worker_main(int argc, char **argv)
{
  if (argc < 3) {
    printf("Invalid call\n");
    return 2;
  }
  sleep(3);
  int t = (int)strtol(argv[2], NULL, 0);

  printf("Process %ld is sleeping in %ld second(s).\n", getpid(), t);
  sleep(t);
  return 0;
}

static volatile bool interrupted = false;
static volatile int worker_exit_pid = 0;
static volatile int worker_exit_status = 0;
static volatile int child_count = 0;

static void kash_handler_on_interrupt(int signo)
{
  interrupted = true;
}

static void kash_handler_on_worker_exit(int signo)
{
  int saved = errno;
  worker_exit_pid = wait((int *)&worker_exit_status);
  child_count--;
  errno = saved;
}

static int spawn_worker(void)
{
  double t;
  char *sleeptime = (char *)malloc(5);

  srandom(getpid());
  t = (double)random() / RAND_MAX * 10;
  asprintf(&sleeptime, "%ld", (int)ceil(t));
  printf("%s\n", sleeptime);

  if (execle("/proc/self/exe", "kashiwa: worker", "test002", sleeptime, NULL, NULL) < 0) {
    perror("execle consumer");
    exit(1);
  }

  return 0;
}

static int kash_do_consumer_main(int argc, char **argv)
{
  if (argc < 1) {
    printf("Invalid call\n");
    return 2;
  }

  char *queue_name = argv[1];

  struct sigaction act = {
      .sa_handler = kash_handler_on_worker_exit, .sa_flags = 0,
  };
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGCHLD, &act, NULL) < 0) {
    perror("sigaction");
    return 1;
  }

  struct timespec ts = {
      .tv_sec = 0, .tv_nsec = 500000000,
  };

  for (;;) {
    nanosleep(&ts, NULL);

    if (worker_exit_pid) {
      printf("Worker is exited: PID=%ld, exit_status=%ld\n", worker_exit_pid, worker_exit_status);
      worker_exit_pid = 0;
    }

    if (child_count >= 10) {
      continue;
    }

    pid_t pid = fork();
    switch (pid) {
    case -1:
      perror("fork");
      exit(2);
      break;
    case 0:
      if (spawn_worker() < 0) {
        exit(1);
      }
      break;
    }
    child_count++;
  }
}

static int kash_do_root_main(int argc, char **argv)
{
  char *queue_name = "test001";

  pid_t pid = fork();
  switch (pid) {
  case -1:
    perror("fork");
    exit(2);
  case 0:
    if (execle("/proc/self/exe", "kashiwa: consumer", queue_name, NULL, NULL) < 0) {
      perror("execle root");
      exit(1);
    }
  }

  int exit_status;
  if (waitpid(pid, &exit_status, 0) < 0) {
    perror("waitpid");
    return 2;
  }

  printf("consumer exited: %ld\n", exit_status);
  if (exit_status == 0) {
    return 0;
  } else {
    return 2;
  }
}

int main(int argc, char **argv)
{
  int exitcode;
  const char *progname = argv[0];
  if (memcmp(progname, "kashiwa: consumer", strlen("kashiwa: consumer") - 1) == 0) {
    exitcode = kash_do_consumer_main(argc, argv);
  } else if (memcmp(progname, "kashiwa: worker", strlen("kashiwa: worker") - 1) == 0) {
    exitcode = kash_do_worker_main(argc, argv);
  } else {
    exitcode = kash_do_root_main(argc, argv);
  }

  exit(exitcode);
}
