#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* Signale une erreur fatale */
void error(const char *fmt, ...)
{
	va_list valist;

	va_start(valist, fmt);

	vfprintf(stderr, fmt, valist);
	exit(EXIT_FAILURE);

	va_end(valist);
}
