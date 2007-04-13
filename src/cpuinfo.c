/*
 *  cpuinfo.c - Processor identification
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
#include "cpuinfo.h"

#define DEBUG 0
#include "debug.h"

static void print_usage(const char *progname)
{
  printf("cpuinfo, get processor information.  Version %s\n", CPUINFO_VERSION);
  printf("\n");
  printf("  usage: %s [<options>]\n", progname);
  printf("\n");
  printf("   -h --help               print this message\n");
  printf("   -d --debug [FILE]       dump debug information into FILE\n");
}

static void print_cpuinfo(struct cpuinfo *cip, FILE *out)
{
  int i, j;

  fprintf(out, "Processor Information\n");

  int vendor = cpuinfo_get_vendor(cip);
  fprintf(out, "  Model: %s %s", cpuinfo_string_of_vendor(vendor), cpuinfo_get_model(cip));
  int freq = cpuinfo_get_frequency(cip);
  if (freq > 0) {
	fprintf(out, ", ");
	if (freq > 1000)
	  fprintf(out, "%.2f GHz", (double)freq / 1000.0);
	else
	  fprintf(out, "%d MHz", freq);
  }
  fprintf(out, "\n");

  int socket = cpuinfo_get_socket(cip);
  fprintf(out, "  Package:");
  if (socket != CPUINFO_SOCKET_UNKNOWN)
	fprintf(out, " %s,", cpuinfo_string_of_socket(socket));
  int n_cores = cpuinfo_get_cores(cip);
  fprintf(out, " %d Core%s", n_cores, n_cores > 1 ? "s" : "");
  int n_threads = cpuinfo_get_threads(cip);
  if (n_threads > 1)
	fprintf(out, ", %d Threads per Core", n_threads);
  fprintf(out, "\n");

  fprintf(out, "\n");
  fprintf(out, "Processor Caches\n");

  const cpuinfo_cache_t *ccp = cpuinfo_get_caches(cip);
  if (ccp) {
	for (i = 0; i < ccp->count; i++) {
	  const cpuinfo_cache_descriptor_t *ccdp = &ccp->descriptors[i];
	  if (ccdp->level == 0 && ccdp->type == CPUINFO_CACHE_TYPE_TRACE)
		fprintf(out, "  Trace cache, %dK uOps", ccdp->size);
	  else {
		fprintf(out, "  L%d %s cache, ", ccdp->level, cpuinfo_string_of_cache_type(ccdp->type));
		if (ccdp->size >= 1024) {
		  if ((ccdp->size % 1024) == 0)
			fprintf(out, "%d MB", ccdp->size / 1024);
		  else
			fprintf(out, "%.2f MB", (double)ccdp->size / 1024.0);
		}
		else
		  fprintf(out, "%d KB", ccdp->size);
	  }
	  fprintf(out, "\n");
	}
  }

  fprintf(out, "\n");
  fprintf(out, "Processor Features\n");

  static const struct {
	int base;
	int max;
  } features_bits[] = {
	{ CPUINFO_FEATURE_COMMON + 1, CPUINFO_FEATURE_COMMON_MAX },
	{ CPUINFO_FEATURE_X86, CPUINFO_FEATURE_X86_MAX },
	{ CPUINFO_FEATURE_PPC, CPUINFO_FEATURE_PPC_MAX },
	{ -1, 0 }
  };
  for (i = 0; features_bits[i].base != -1; i++) {
	int base = features_bits[i].base;
	int count = features_bits[i].max - base;
	for (j = 0; j < count; j++) {
	  int feature = base + j;
	  if (cpuinfo_has_feature(cip, feature)) {
		const char *name = cpuinfo_string_of_feature(feature);
		const char *detail = cpuinfo_string_of_feature_detail(feature);
		fprintf(out, "  %-10s %s\n", name, detail);
	  }
	}
  }
}

int main(int argc, char *argv[])
{
  int i;
  FILE *out;
  const char *out_filename = NULL;

  for (i = 1; i < argc; i++) {
	const char *arg = argv[i];
	if (strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0) {
	  if (++i < argc)
		out_filename = argv[i];
	  else
		out_filename = "-"; /* stdout */
	}
	else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
	  print_usage(argv[0]);
	  return 0;
	}
  }

  struct cpuinfo *cip = cpuinfo_new();
  if (cip == NULL) {
	fprintf(stderr, "ERROR: could not allocate cpuinfo descriptor\n");
	return 1;
  }

  if (out_filename == NULL || strcmp(out_filename, "-") == 0)
	out = stdout;
  else {
	if ((out = fopen(out_filename, "w")) == NULL) {
	  fprintf(stderr, "ERROR: could not open debug file '%s'\n", out_filename);
	  return 2;
	}
  }

  print_cpuinfo(cip, out);

  if (out_filename) { /* debug mode */
	fprintf(out, "\n### DEBUGGING INFORMATION ###\n\n");
	cpuinfo_dump(cip, out);
  }

  if (out != stdout)
	fclose(out);

  cpuinfo_destroy(cip);

  return 0;
}
