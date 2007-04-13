/*
 *  debug.c - Debugging utilities
 *
 *  cpuinfo (C) 2006-2007 Gwenole Beauchesne
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include <stdarg.h>
#include "debug.h"


static FILE *g_debug_file = NULL;

void cpuinfo_set_debug_file(FILE *debug_file)
{
  g_debug_file = debug_file;
}

void cpuinfo_dprintf(const char *format, ...)
{
  if (g_debug_file) {
	va_list args;
	va_start(args, format);
	fprintf(g_debug_file, "* ");
	vfprintf(g_debug_file, format, args);
	va_end(args);
  }
}
