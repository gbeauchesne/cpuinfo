/*
 *  cpuinfo.c - Processor identification
 *
 *  cpuinfo (C) 2006 Gwenole Beauchesne
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

#include <stdio.h>
#include "cpuinfo.h"

#define DEBUG 0
#include "debug.h"

int main(void)
{
  int i, j;

  struct cpuinfo *cip = cpuinfo_new();
  if (cip == NULL) {
	fprintf(stderr, "ERROR: could not allocate cpuinfo descriptor\n");
	return 1;
  }

  printf("Processor Information\n");

  int vendor = cpuinfo_get_vendor(cip);
  printf("  Model: %s %s", cpuinfo_string_of_vendor(vendor), cpuinfo_get_model(cip));
  int freq = cpuinfo_get_frequency(cip);
  if (freq > 0) {
	printf(", ");
	if (freq > 1000)
	  printf("%.2f GHz", (double)freq / 1000.0);
	else
	  printf("%d MHz", freq);
  }
  printf("\n");

  int socket = cpuinfo_get_socket(cip);
  printf("  Package:");
  if (socket != CPUINFO_SOCKET_UNKNOWN)
	printf(" %s,", cpuinfo_string_of_socket(socket));
  int n_cores = cpuinfo_get_cores(cip);
  printf(" %d Core%s", n_cores, n_cores > 1 ? "s" : "");
  int n_threads = cpuinfo_get_threads(cip);
  if (n_threads > 1)
	printf(", %d Threads per Core", n_threads);
  printf("\n");

  printf("\n");
  printf("Processor Caches\n");

  const cpuinfo_cache_t *ccp = cpuinfo_get_caches(cip);
  if (ccp) {
	for (i = 0; i < ccp->count; i++) {
	  const cpuinfo_cache_descriptor_t *ccdp = &ccp->descriptors[i];
	  if (ccdp->level == 0 && ccdp->type == CPUINFO_CACHE_TYPE_TRACE)
		printf("  Trace cache, %dK uOps", ccdp->size);
	  else {
		printf("  L%d %s cache, ", ccdp->level, cpuinfo_string_of_cache_type(ccdp->type));
		if (ccdp->size > 1024)
		  printf("%.2f MB", (double)ccdp->size / 1024.0);
		else
		  printf("%d KB", ccdp->size);
	  }
	  printf("\n");
	}
  }

  printf("\n");
  printf("Processor Features\n");

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
		printf("  %-10s %s\n", name, detail);
	  }
	}
  }

  cpuinfo_destroy(cip);

  return 0;
}
