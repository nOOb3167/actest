#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include <error.h>

void
xexit (const char *what)
{
    printf ("ERROR: %s\n", what);
    exit (1);
}

void
xfake_on_error_stack_trace(const char *a, ...)
{
    g_on_error_stack_trace("Abcdef");
}
