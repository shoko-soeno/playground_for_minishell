#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "minishell.h"

bool syntax_error = false;

void fatal_error (const char *msg)
{
    dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
    exit(1);
}

void assert_error (const char *msg)
{
    dprintf(STDERR_FILENO, "Assertion Error: %s\n", msg);
    exit(1);
}

void err_exit (const char *location, const char *msg, int status)
{
    dprintf(STDERR_FILENO, "minishell: %s: %s\n", location, msg);
    exit(status);
}

void todo (const char *msg)
{
    dprintf(STDERR_FILENO, "TODO: %s\n", msg);
    exit(1);
}

void tokenize_error (const char *location, char **reset, char *line)
{
    syntax_error = true;
    dprintf(STDERR_FILENO, "minishell: syntax error near `%s'\n", location);
    dprintf(STDERR_FILENO, "%s\n", line);
    dprintf(STDERR_FILENO, reset ? "reset: %s\n" : "reset: NULL\n", reset ? *reset : NULL);
    while (*line)
        line++;
    *reset = line; //ここがNULLになっていたのでセグフォしてた
}