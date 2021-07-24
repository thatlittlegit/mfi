/*
 * Copyright (C) 2021 thatlittlegit
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE

#include "config.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define REQUIRED_FDS 7

static char *DEFAULT_COMMAND[] = { "/bin/echo", "hello, world", NULL };
static char **command = DEFAULT_COMMAND;

#define DISTRO_FAULT 1
enum fail_reason
{
  FAIL_INAPPROPRIATE = 0,
  FAIL_COULDNTSPAWN = 1,
  FAIL_COULDNTSTDIO = 2,
  FAIL_COULDNTSIGNAL = 4,
  FAIL_FATALSIGNAL = 6,
  FAIL_RLIMITS = 8,
  _FAIL_LAST
};

const char *const ERROR_MESSAGES[] = {
  "not running as PID1, consider '--fake' (see mfi(1))",
  "couldn't spawn main command (is it missing?)",
  "couldn't configure file descriptors",
  NULL,
  "couldn't initialize signal handlers",
  NULL,
  "received a fatal signal",
  NULL,
  "system resource limits were too low",
};

struct arguments
{
  int no_sigint;
};

static void
help (const char *progname)
{
  assert (progname != NULL);

  fprintf (stdout,
           "mfi (minimum feasable init) is a tiny program that "
           "acts as PID1 and tries not to\ndie, to prevent kernel panics.\n"
           "\n"
#if MFI_CUSTOM_COMMANDS
           "Usage: %s [-hVc] [command...]\n"
#else
           "Usage: %s [-hVc]\n"
#endif
           "\n"
           "Options:\n"
           "\n"
           "  --help, -h            print this help information\n"
           "  --version, -V         print the version number and license\n",
           progname);

  fprintf (stdout,
           "  --command, -c         print the command that will be run on "
           "startup\n"
           "  --no-signals, -S      don't connect signal handlers for SIGINT\n"
           "\n"
           "Report bugs to <" PACKAGE_BUGREPORT ">.\n");
}

static void
version (void)
{
  fputs (PACKAGE_NAME " " PACKAGE_VERSION "\n\n"
                      "Copyright (C) 2021 thatlittlegit\n"
                      "This is free software; see the source for copying "
                      "conditions.  There is NO\n"
                      "warranty; not even for MERCHANTABILITY or FITNESS FOR "
                      "A PARTICULAR PURPOSE.\n",
         stdout);
}

static void
print_command (void)
{
  int i;

  for (i = 0; command[i] != NULL; ++i)
    {
      fputs (command[i], stdout);
      fputc (' ', stdout);
    }

  fputc ('\n', stdout);
}

static int
set_command (int argc, char **argv)
{
  assert (argc > 0);
  assert (optind < argc);

  if (MFI_CUSTOM_COMMANDS)
    {
      command = argv + optind;
      return 1;
    }

  fprintf (stderr, "%s: your administrator has disabled custom commands\n",
           argv[0]);
  return -1;
}

static int
parse_arguments (int argc, char **argv, struct arguments *args)
{
  static const struct option OPTIONS[]
      = { { "help", no_argument, NULL, 'h' },
          { "version", no_argument, NULL, 'V' },
          { "command", no_argument, NULL, 'c' },
          { "no-signals", no_argument, NULL, 'S' },
          { NULL, 0, 0, 0 } };

  assert (argc > 0);

  for (;;)
    {
      int chr = getopt_long (argc, argv, ":hVcS", OPTIONS, NULL);
      if (chr < 0)
        break;

      switch (chr)
        {
        case 'h':
          help (argv[0]);
          return 0;
        case 'V':
          version ();
          return 0;
        case 'c':
          print_command ();
          return 0;
        case 'S':
          args->no_sigint = 1;
          continue;
        default:
          fprintf (stderr,
                   "Unknown parameter -%c\n"
                   "Usage: %s [-hVc]\n",
                   optopt, argv[0]);
          return -1;
        }
    }

  if (optind < argc)
    return set_command (argc, argv);

  return 1;
}

#define fail(r) fail_ex (r, "(no additional information is available)")
static void
fail_ex (enum fail_reason fail_reason, const char *additional)
{
  const char *fault
      = (fail_reason & DISTRO_FAULT) ? "probably an mfi bug, " : "";

#if MFI_INIT_MESSAGE_STYLE
  static const struct timespec spec = { 5, 0 };

  fprintf (
      stderr,
      "\n"
      "mfi: a critical error has occurred, and your computer must reset.\n"
      "mfi:\n"
      "mfi: error code MFI-%d (%scontact your distributor)\n"
      "mfi:  > %s\n"
      "mfi:\n"
      "mfi: errno = %d (%s)\n"
      "mfi:\n"
      "mfi: %s\n"
      "mfi:\n"
      "mfi: mfi will exit, maybe panicking your kernel, in five seconds.\n",
      fail_reason, fault, ERROR_MESSAGES[fail_reason], errno, strerror (errno),
      additional);

  nanosleep (&spec, NULL);
  raise (SIGABRT);
  exit (EXIT_FAILURE);
#else
  fprintf (stderr,
           "mfi: error MFI-%d [%s]\n"
           "mfi: errno is %d (%s)\n"
           "mfi: %s\n"
           "mfi: exiting, %scontact your distribution for support\n",
           fail_reason, ERROR_MESSAGES[fail_reason], errno, strerror (errno),
           additional, fault);
  exit (EXIT_FAILURE);
#endif
}

static void
fatal_signal (int signum, siginfo_t *info, void *context)
{
  static char text[64];
  (void)context;

  memset (text, 0, sizeof (text));

  switch (signum)
    {
    case SIGSEGV:
    case SIGBUS:
      snprintf (text, sizeof (text), "(faulted at %p)", info->si_addr);
      break;
    case SIGFPE:
      snprintf (text, sizeof (text), "(floating-point exception?!)");
      break;
    case SIGILL:
      snprintf (text, sizeof (text), "(illegal instruction at %p)",
                info->si_addr);
      break;
    case SIGABRT:
      snprintf (text, sizeof (text), "(assertion failure)");
      break;
    default:
      snprintf (text, sizeof (text), "(unknown signal #%d)", signum);
      break;
    }

  fail_ex (FAIL_FATALSIGNAL, text);
}

static int
setup_signals (int include_sigint)
{
  sigset_t block;
  struct sigaction action;

  sigemptyset (&block);
  sigfillset (&block);

  /* we can't catch anyways */
  sigdelset (&block, SIGKILL);
  sigdelset (&block, SIGSTOP);

  /* unspecified to block */
  sigdelset (&block, SIGBUS);
  sigdelset (&block, SIGFPE);
  sigdelset (&block, SIGILL);
  sigdelset (&block, SIGSEGV);

  /* we're making ourselves crash */
  sigdelset (&block, SIGABRT);

  /* user interrupt */
  if (!include_sigint)
    sigdelset (&block, SIGINT);

  sigprocmask (SIG_SETMASK, &block, NULL);

  action.sa_sigaction = fatal_signal;
  action.sa_mask = block;
  action.sa_flags = SA_SIGINFO;
  action.sa_restorer = NULL;
  sigaction (SIGBUS, &action, NULL);
  sigaction (SIGFPE, &action, NULL);
  sigaction (SIGILL, &action, NULL);
  sigaction (SIGSEGV, &action, NULL);
  sigaction (SIGABRT, &action, NULL);

  return 0;
}

static int
check_rlimits (void)
{
  struct rlimit rlim;

  /* only failures are EFAULT or EINVAL, neither of which should apply */
  if (getrlimit (RLIMIT_NOFILE, &rlim) < 0)
    return -1;

  if (rlim.rlim_cur > REQUIRED_FDS)
    return 0;

  if (rlim.rlim_max < REQUIRED_FDS)
    return -1;

  rlim.rlim_cur = rlim.rlim_max;
  if (setrlimit (RLIMIT_NOFILE, &rlim) < 0)
    return -1;

  return 0;
}

static int
setup_stdio (int *consolefd_in, int *consolefd_out, int *commfd)
{
  int pipes[2];
  int consolefd_out_;
  int consolefd_in_;

  assert (consolefd_in != NULL);
  assert (consolefd_out != NULL);
  assert (commfd != NULL);

  consolefd_out_ = dup (STDOUT_FILENO);
  if (consolefd_out_ < 0)
    goto cleanup_none;

  consolefd_in_ = dup (STDIN_FILENO);
  if (consolefd_in_ < 0)
    goto cleanup_consolefd_out;

  if (pipe2 (pipes, O_CLOEXEC) < 0)
    goto cleanup_consolefd_in;

  if (dup3 (STDOUT_FILENO, STDERR_FILENO, O_CLOEXEC) < 0)
    goto cleanup_pipes;

  if (dup3 (pipes[1], STDOUT_FILENO, O_CLOEXEC) < 0)
    goto cleanup_pipes;

  close (STDIN_FILENO);

  *consolefd_out = consolefd_out_;
  *consolefd_in = consolefd_in_;
  *commfd = pipes[0];

  return 0;

cleanup_pipes:
  close (pipes[0]);
  close (pipes[1]);
cleanup_consolefd_in:
  close (consolefd_in_);
cleanup_consolefd_out:
  close (consolefd_out_);
cleanup_none:
  return -1;
}

static pid_t
spawn_command (int stdinput, int stdoutput, int commfd, char *const command[])
{
  int result;
  pid_t ret;
  /* TODO allocate and set up at startup */
  posix_spawn_file_actions_t actions;

  posix_spawn_file_actions_init (&actions);
  posix_spawn_file_actions_adddup2 (&actions, stdinput, STDIN_FILENO);
  posix_spawn_file_actions_adddup2 (&actions, stdoutput, STDOUT_FILENO);
  posix_spawn_file_actions_adddup2 (&actions, stdoutput, STDERR_FILENO);
  posix_spawn_file_actions_adddup2 (&actions, commfd, 3);

  result = posix_spawn (&ret, command[0], &actions, NULL, command, NULL);

  posix_spawn_file_actions_destroy (&actions);

  if (result < 0)
    return result;

  return ret;
}

int
main (int argc, char **argv)
{
  struct arguments args;
  int commfd;
  int consolefd_in;
  int consolefd_out;
  pid_t special_pid = 0;

  memset (&args, 0, sizeof (struct arguments));

  if (argc > 1)
    {
      int ret = parse_arguments (argc, argv, &args);
      if (ret <= 0)
        return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  if (check_rlimits () < 0)
    fail (FAIL_RLIMITS);

  if (setup_signals (!args.no_sigint) < 0)
    fail (FAIL_COULDNTSIGNAL);

  if (setup_stdio (&consolefd_in, &consolefd_out, &commfd) < 0)
    fail (FAIL_COULDNTSTDIO);

  for (;;)
    {
      pid_t problem_child = wait (NULL);

      if (problem_child == special_pid || errno == ECHILD)
        {
          special_pid
              = spawn_command (consolefd_in, consolefd_out, commfd, command);
          if (special_pid < 0)
            fail (FAIL_COULDNTSPAWN);
        }
    }
}
