/*
 *  cpuinfo-ia64.c - Processor identification code, ia64 specific
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
#include "cpuinfo-private.h"

#if defined __hpux
#include <unistd.h>
#include <sys/pstat.h>
#endif

#define DEBUG 1
#include "debug.h"

// Extract CPUID registers
static uint64_t cpuid(int reg)
{
  uint64_t value = 0;
#if defined __GNUC__
  __asm__ __volatile__ ("mov %0=cpuid[%1]" : "=r" (value) : "r" (reg));
#elif defined __HP_cc || defined __HP_aCC
  value = _Asm_mov_from_cpuid(reg);
#endif
  return value;
}

// Arch-dependent data
#define N_CPUID_REGISTERS 5
struct ia64_cpuinfo {
  uint64_t cpuid[N_CPUID_REGISTERS];
  cpuinfo_list_t caches;
  int frequency;
  uint32_t features[CPUINFO_FEATURES_SZ_(IA64)];
};

typedef struct ia64_cpuinfo ia64_cpuinfo_t;

// Initialize arch-dependent cpuinfo datastructure
static int cpuinfo_arch_init(ia64_cpuinfo_t *acip)
{
  memset(&acip->cpuid, 0, sizeof(acip->cpuid));
  acip->caches = NULL;
  acip->frequency = 0;
  memset(&acip->features, 0, sizeof(acip->features));

  for (int i = 0; i <= 3; i++)
	acip->cpuid[i] = cpuid(i);
  if ((acip->cpuid[3] & 0xff) >= 4)
	acip->cpuid[4] = cpuid(4);

  if (acip->cpuid[3] == 0)
	return -1;

  // Determine caches hierarchy
#if defined __linux__
  char line[256];
  char dummy[256];
  cpuinfo_cache_descriptor_t cache_desc;
  FILE *cache_info = fopen("/proc/pal/cpu0/cache_info", "r"); // XXX: iterate until an online processor
  if (cache_info) {
	char cache_type[32];
	cache_desc.level = -1;
	while(fgets(line, sizeof(line), cache_info)) {
	  // Read line
	  int len = strlen(line);
	  if (len == 0)
		continue;
	  line[len-1] = 0;

	  // Parse line
	  int i;
	  if (sscanf(line, "%s Cache level %d", cache_type, &i) == 2) {
		if (cache_desc.level > 0)
		  cpuinfo_list_insert(&acip->caches, &cache_desc);
		cache_desc.level = i;
		if (strcmp(cache_type, "Instruction") == 0)
		  cache_desc.type = CPUINFO_CACHE_TYPE_CODE;
		else if (strcmp(cache_type, "Data") == 0)
		  cache_desc.type = CPUINFO_CACHE_TYPE_DATA;
		else if (strcmp(cache_type, "Data/Instruction") == 0)
		  cache_desc.type = CPUINFO_CACHE_TYPE_UNIFIED;
		else
		  fprintf(stderr, "ERROR: unknown cache type '%s'\n", cache_type);
	  }
	  else if (sscanf(line, "%[ \t] Size : %d bytes", dummy, &i) == 2) {
		cache_desc.size = i / 1024;
	  }
	}
	if (cache_desc.level > 0)
	  cpuinfo_list_insert(&acip->caches, &cache_desc);
	fclose(cache_info);
  }
#endif

  // Determine CPU clock frequency
#if defined __linux__
  FILE *proc_file = fopen("/proc/cpuinfo", "r");
  if (proc_file) {
	while(fgets(line, sizeof(line), proc_file)) {
	  // Read line
	  int len = strlen(line);
	  if (len == 0)
		continue;
	  line[len-1] = 0;

	  // Parse line
	  float f;
	  if (sscanf(line, "cpu MHz : %fMHz", &f) == 1)
		acip->frequency = (int)f;
	}
  }
#elif defined __hpux
  struct pst_processor proc;
  if (pstat_getprocessor(&proc, sizeof(proc), 1, 0) == 1) {
	long clk_tck;
	if ((clk_tck = sysconf(_SC_CLK_TCK)) != -1)
	  acip->frequency = proc.psp_iticksperclktick * clk_tck / 1000000;
  }
#endif

  return 0;
}

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  ia64_cpuinfo_t *p = (ia64_cpuinfo_t *)malloc(sizeof(*p));
  if (p == NULL)
	return -1;
  if (cpuinfo_arch_init(p) < 0) {
	free(p);
	return -1;
  }
  cip->opaque = p;
  return 0;
}

// Release the cpuinfo descriptor and all allocated data
void cpuinfo_arch_destroy(struct cpuinfo *cip)
{
  ia64_cpuinfo_t *acip = (ia64_cpuinfo_t *)(cip->opaque);
  if (acip) {
	if (acip->caches)
	  cpuinfo_list_clear(&acip->caches);
	free(acip);
  }
}

// Dump all useful information for debugging
int cpuinfo_dump(struct cpuinfo *cip, FILE *out)
{
  if (cip == NULL)
	return -1;

  ia64_cpuinfo_t *acip = (ia64_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return -1;

  fprintf(out, "%-10s : %d\n", "family", (int)((acip->cpuid[3] >> 24) & 0xff));
  fprintf(out, "%-10s : %d\n", "model", (int)((acip->cpuid[3] >> 16) & 0xff));
  fprintf(out, "%-10s : %d\n", "revision", (int)((acip->cpuid[3] >> 8) & 0xff));
  fprintf(out, "%-10s : %d\n", "archrev", (int)((acip->cpuid[3] >> 32) & 0xff));
  fprintf(out, "\n");

  for (int i = 0; i < N_CPUID_REGISTERS; i++) {
	fprintf(out, "CPUID Register %d : %08x%08x\n", i,
			(uint32_t)(acip->cpuid[i] >> 32), (uint32_t)acip->cpuid[i]);
  }

  return 0;
}

// Get processor vendor ID 
int cpuinfo_arch_get_vendor(struct cpuinfo *cip)
{
  ia64_cpuinfo_t *acip = (ia64_cpuinfo_t *)(cip->opaque);
  if (acip) {
	if (acip->cpuid[0] == 0x49656e69756e6547ULL && 0x6c65746eULL)
	  return CPUINFO_VENDOR_INTEL;
  }

  return CPUINFO_VENDOR_UNKNOWN;
}

// Get processor name
char *cpuinfo_arch_get_model(struct cpuinfo *cip)
{
  ia64_cpuinfo_t *acip = (ia64_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return NULL;

  uint64_t vi = acip->cpuid[3];
  int archrev = (vi >> 32) & 0xff;
  int family = (vi >> 24) & 0xff;
  int model = (vi >> 16) & 0xff;
  int revision = (vi >> 8) & 0xff;

  const char *name = NULL;
  const char *codename = NULL;
  switch (family) {
  case 0x07:
	name = "Itanium";
	codename = "Merced";
	break;
  case 0x1f:
	name = "Itanium 2";
	switch (model) {
	case 1:
	  codename = "McKinley";
	  break;
	case 2:
	  codename = "Madison 6M";
	  break;
	case 3:
	  codename = "Madison 9M";
	  break;
	}
	break;
  case 0x20:
	name = "Itanium 2";
	codename = "Montecito";
	break;
  }

  if (name) {
	int model_length = strlen(name) + 1;
	if (codename)
	  model_length += strlen(codename) + 3;
	char *model = (char *)malloc(model_length);
	if (model) {
	  if (codename)
		sprintf(model, "%s '%s'", name, codename);
	  else
		strcpy(model, name);
	}
	return model;
  }

  return NULL;
}

// Get processor frequency in MHz
int cpuinfo_arch_get_frequency(struct cpuinfo *cip)
{
  return ((ia64_cpuinfo_t *)(cip->opaque))->frequency;
}

// Get processor socket ID
int cpuinfo_arch_get_socket(struct cpuinfo *cip)
{
  return -1;
}

// Get number of cores per CPU package
int cpuinfo_arch_get_cores(struct cpuinfo *cip)
{
  return -1;
}

// Get number of threads per CPU core
int cpuinfo_arch_get_threads(struct cpuinfo *cip)
{
  return -1;
}

// Get cache information
cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip)
{
  return ((ia64_cpuinfo_t *)(cip->opaque))->caches;
}

// Returns features table
uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature)
{
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	return cip->features;
  case CPUINFO_FEATURE_IA64:
	return ((ia64_cpuinfo_t *)(cip->opaque))->features;
  }
  return NULL;
}

// Returns 0 if CPU supports the specified feature
int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature)
{
  if (!cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_IA64)) {
	cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_IA64);

	uint64_t features = ((ia64_cpuinfo_t *)(cip->opaque))->cpuid[4];
	if (features & (1 << 0))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_IA64_LB);
	if (features & (1 << 1))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_IA64_SD);
	if (features & (1 << 2))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_IA64_AO);
  }

  return cpuinfo_feature_get_bit(cip, feature);
}
