#include <assert.h>
#include <getopt.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/* NOTE this is not const because posix_spawn demands a char *const[] */
static char *const MFI_COMMAND[] = { "/bin/echo", "hello, world", NULL };

static void
help (const char *progname)
{
  assert (progname != NULL);

  fprintf (
      stdout,
      "mfi (minimum feasable init) is a tiny program that "
      "acts as PID1 and tries not to\ndie, to prevent kernel panics.\n"
      "\n"
      "Usage: %s [-hVc]\n"
      "\n"
      "Options:\n"
      "\n"
      "  --help, -h            print this help information\n"
      "  --version, -V         print the version number and license\n"
      "  --command, -c         print the command that will be run on startup\n"
      "\n"
      "Report bugs to <https://github.com/thatlittlegit/mfi>.\n",
      progname);
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

  for (i = 0; MFI_COMMAND[i] != NULL; ++i)
    {
      fputs (MFI_COMMAND[i], stdout);
      fputc (' ', stdout);
    }

  fputc ('\n', stdout);
}

static int
parse_arguments (int argc, char **argv)
{
  static const struct option OPTIONS[]
      = { { "help", no_argument, NULL, 'h' },
          { "version", no_argument, NULL, 'V' },
          { "command", no_argument, NULL, 'c' },
          { NULL, 0, 0, 0 } };

  assert (argc > 0);

  for (;;)
    {
      int chr = getopt_long (argc, argv, ":hVc", OPTIONS, NULL);
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
        default:
          fprintf (stderr,
                   "Unknown parameter -%c\n"
                   "Usage: %s [-hVc]\n",
                   optopt, argv[0]);
          return -1;
        }
    }
}

static pid_t
spawn_command (char *const command[])
{
  int result;
  pid_t ret;

  result = posix_spawn (&ret, command[0], NULL, NULL, command, NULL);

  if (result < 0)
    return result;

  return ret;
}

int
main (int argc, char **argv)
{
  pid_t special_pid;
  pid_t problem_child;

  if (argc > 1)
    {
      int ret = parse_arguments (argc, argv);
      if (ret <= 0)
        return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

  special_pid = spawn_command (MFI_COMMAND);
  if (special_pid < 0)
    return EXIT_FAILURE;

  for (;;)
    {
      problem_child = wait (NULL);
      if (problem_child != special_pid)
        continue;

      special_pid = spawn_command (MFI_COMMAND);
      if (special_pid < 0)
        return EXIT_FAILURE;
    }
}
