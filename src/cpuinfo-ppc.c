/*
 *  cpuinfo-ppc.c - Processor identification code, ppc specific
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#if defined __APPLE__ && defined __MACH__
#include <mach-o/arch.h>
#endif

#define DEBUG 1
#include "debug.h"

#if defined __powerpc__ || defined __ppc__

// Arch-dependent data
struct ppc_cpuinfo {
  uint32_t pvr;
  uint32_t frequency;
  uint32_t features[CPUINFO_FEATURES_SZ_(PPC)];
};

typedef struct ppc_cpuinfo ppc_cpuinfo_t;

// CPU caches specifications
#define DEFINE_CACHE_DESCRIPTOR(NAME, TYPE, LEVEL, SIZE) \
static const cpuinfo_cache_descriptor_t NAME = { CPUINFO_CACHE_TYPE_##TYPE, LEVEL, SIZE }
DEFINE_CACHE_DESCRIPTOR(L1I_16KB,	CODE,		1,	  16);
DEFINE_CACHE_DESCRIPTOR(L1I_32KB,	CODE,		1,	  32);
DEFINE_CACHE_DESCRIPTOR(L1D_16KB,	DATA,		1,	  16);
DEFINE_CACHE_DESCRIPTOR(L1D_32KB,	DATA,		1,	  32);
DEFINE_CACHE_DESCRIPTOR(L2_512KB,	UNIFIED,	2,	 512);
DEFINE_CACHE_DESCRIPTOR(L2_1MB,		UNIFIED,	2,	1024);
#undef DEFINE_CACHE_DESCRIPTOR

// CPU specs table
#define N_CACHE_DESCRIPTORS 4
struct ppc_spec_entry {
  uint32_t pvr_mask;
  uint32_t pvr_value;
  int vendor;
  char *model;
  int n_cores;
  int n_threads;
  const cpuinfo_cache_descriptor_t *caches[N_CACHE_DESCRIPTORS];
};

typedef struct ppc_spec_entry ppc_spec_t;

static const ppc_spec_t ppc_specs[] = {
  { /* 601 */
	0xffff0000, 0x00010000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 601",
	1, 1,
	{ NULL, }
  },
  { /* 603 */
	0xffff0000, 0x00030000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603",
	1, 1,
	{ NULL, }
  },
  { /* 603e */
	0xffff0000, 0x00060000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603e",
	1, 1,
	{ NULL, }
  },
  { /* 603ev */
	0xffff0000, 0x00070000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603ev",
	1, 1,
	{ NULL, }
  },
  { /* 604 */
	0xffff0000, 0x00040000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604",
	1, 1,
	{ NULL, }
  },
  { /* 604e */
	0xfffff000, 0x00090000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604e",
	1, 1,
	{ NULL, }
  },
  { /* 604r */
	0xffff0000, 0x00090000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604r",
	1, 1,
	{ NULL, }
  },
  { /* 604ev */
	0xffff0000, 0x000a0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604ev",
	1, 1,
	{ NULL, }
  },
  { /* 750CX */
	0xfffffff0, 0x00080100,
	CPUINFO_VENDOR_IBM, "PowerPC 750CX",
	1, 1,
	{ NULL, }
  },
  { /* 750CX */
	0xfffffff0, 0x00082200,
	CPUINFO_VENDOR_IBM, "PowerPC 750CX",
	1, 1,
	{ NULL, }
  },
  { /* 750CXe */
	0xfffffff0, 0x00082210,
	CPUINFO_VENDOR_IBM, "PowerPC 750CXe",
	1, 1,
	{ NULL, }
  },
  { /* 750FX */
	0xffff0000, 0x70000000,
	CPUINFO_VENDOR_IBM, "PowerPC 750FX",
	1, 1,
	{ NULL, }
  },
  { /* 750GX */
	0xffff0000, 0x70020000,
	CPUINFO_VENDOR_IBM, "PowerPC 750GX",
	1, 1,
	{ NULL, }
  },
  { /* 750 */
	0xffff0000, 0x00080000,
	CPUINFO_VENDOR_IBM, "PowerPC 750",
	1, 1,
	{ NULL, }
  },
  { /* 7400 */
	0xffff0000, 0x000c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7400",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_1MB, }
  },
  { /* 7410 */
	0xffff0000, 0x800c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7410",
	1, 1,
	{ NULL, }
  },
  { /* 7450 */
	0xffff0000, 0x80000000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7450",
	1, 1,
	{ NULL, }
  },
  { /* 7455 */
	0xffff0000, 0x80010000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7455",
	1, 1,
	{ NULL, }
  },
  { /* 7457 */
	0xffff0000, 0x80020000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7457",
	1, 1,
	{ NULL, }
  },
  { /* 7447A */
	0xffff0000, 0x80030000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7447A",
	1, 1,
	{ NULL, }
  },
  { /* 7448 */
	0xffff0000, 0x80040000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7448",
	1, 1,
	{ NULL, }
  },
  { /* Unknown */
	0x00000000, 0x00000000,
	CPUINFO_VENDOR_UNKNOWN, NULL,
	-1, -1,
	{ NULL, }
  }
};

// Generic Processor Version Register (PVR) values
enum {
  PVR_POWERPC_601		= 0x00010000,
  PVR_POWERPC_603		= 0x00030000,
  PVR_POWERPC_603E		= 0x00060000,
  PVR_POWERPC_603EV		= 0x00070000,
  PVR_POWERPC_604		= 0x00040000,
  PVR_POWERPC_604E		= 0x00090000,
  PVR_POWERPC_750		= 0x00080000,
  PVR_POWERPC_7400		= 0x000c0000,
  PVR_POWERPC_7450		= 0x80000000,
  PVR_POWERPC_970		= 0x00390000,
};

// Returns PVR (Linux only)
static uint32_t get_pvr(void)
{
  uint32_t pvr;
  __asm__ __volatile__ ("mfpvr %0" : "=r" (pvr));
  return pvr;
}

// Initialize arch-dependent cpuinfo data structure
static int cpuinfo_arch_init(ppc_cpuinfo_t *acip)
{
  acip->pvr = 0;
  acip->frequency = 0;
  memset(acip->features, 0, sizeof(acip->features));

  if (cpuinfo_feature_test_function((cpuinfo_feature_test_function_t)get_pvr))
	acip->pvr = get_pvr();

#if defined __APPLE__ && defined __MACH__
  FILE *proc_file = popen("ioreg -c IOPlatformDevice", "r");
  if (proc_file) {
	char line[256];
	int powerpc_node = 0;
	while (fgets(line, sizeof(line) - 1, proc_file)) {
	  // Read line
	  int len = strlen(line);
	  if (len == 0)
		continue;
	  line[len - 1] = 0;

	  // Parse line
	  if (strstr(line, "o PowerPC,"))
		powerpc_node = 1;
	  else if (powerpc_node) {
		uint32_t value;
		char head[256];
		if (sscanf(line, "%[ |]\"cpu-version\" = <%x>", head, &value) == 2) {
		  if (acip->pvr == 0)
			acip->pvr = value;
		}
		else if (sscanf(line, "%[ |]\"clock-frequency\" = <%x>", head, &value) == 2) {
		  if (acip->frequency == 0)
			acip->frequency = value / (1000 * 1000);
		}
		else if (strchr(line, '}'))
		  powerpc_node = 0;
	  }
	}
	fclose(proc_file);
  }

  if (acip->pvr == 0) {
	const NXArchInfo *a = NXGetLocalArchInfo();
	if (a->cputype == CPU_TYPE_POWERPC) {
	  switch (a->cpusubtype) {
	  case CPU_SUBTYPE_POWERPC_601:		acip->pvr = PVR_POWERPC_601;	break;
	  case CPU_SUBTYPE_POWERPC_603:		acip->pvr = PVR_POWERPC_603;	break;
	  case CPU_SUBTYPE_POWERPC_603e:	acip->pvr = PVR_POWERPC_603E;	break;
	  case CPU_SUBTYPE_POWERPC_603ev:	acip->pvr = PVR_POWERPC_603EV;	break;
	  case CPU_SUBTYPE_POWERPC_604:		acip->pvr = PVR_POWERPC_604;	break;
	  case CPU_SUBTYPE_POWERPC_604e:	acip->pvr = PVR_POWERPC_604E;	break;
	  case CPU_SUBTYPE_POWERPC_750:		acip->pvr = PVR_POWERPC_750;	break;
	  case CPU_SUBTYPE_POWERPC_7400:	acip->pvr = PVR_POWERPC_7400;	break;
	  case CPU_SUBTYPE_POWERPC_7450:	acip->pvr = PVR_POWERPC_7450;	break;
	  case 100:							acip->pvr = PVR_POWERPC_970;	break;
	  }
	}
  }
#elif defined __linux__
  FILE *proc_file = fopen("/proc/cpuinfo", "r");
  if (proc_file) {
	char line[256];
	while(fgets(line, 255, proc_file)) {
	  // Read line
	  int len = strlen(line);
	  if (len == 0)
		continue;
	  line[len-1] = 0;

	  // Parse line
	  int i;
	  float f;
	  if (sscanf(line, "clock : %fMHz", &f) == 1)
		acip->frequency = (int)f;
	  else if (sscanf(line, "clock : %dMHz", &i) == 1)
		acip->frequency = i;
	}
	fclose(proc_file);
  }
#else
  return -1;
#endif

  return 0;
}

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  ppc_cpuinfo_t *p = malloc(sizeof(*p));
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
  if (cip->opaque)
	free(cip->opaque);
}

// Get CPU spec
static const ppc_spec_t *get_ppc_spec(struct cpuinfo *cip)
{
  int i;
  const uint32_t pvr = ((ppc_cpuinfo_t *)(cip->opaque))->pvr;
  const int n_specs = sizeof(ppc_specs) / sizeof(ppc_specs[0]);
  for (i = 0; i < n_specs; i++) {
	const ppc_spec_t *spec = &ppc_specs[i];
	if ((pvr & spec->pvr_mask) == spec->pvr_value)
	  return spec;
  }
  return NULL;
}

// Get processor vendor ID 
int cpuinfo_arch_get_vendor(struct cpuinfo *cip)
{
  const ppc_spec_t *spec = get_ppc_spec(cip);
  if (spec)
	return spec->vendor;

  return CPUINFO_VENDOR_UNKNOWN;
}

// Get processor name
char *cpuinfo_arch_get_model(struct cpuinfo *cip)
{
  const ppc_spec_t *spec = get_ppc_spec(cip);
  if (spec && spec->model) {
	char *model = malloc(strlen(spec->model) + 1);
	if (model == NULL)
	  return NULL;
	strcpy(model, spec->model);
	return model;
  }

  return NULL;
}

// Get processor frequency in MHz
int cpuinfo_arch_get_frequency(struct cpuinfo *cip)
{
  ppc_cpuinfo_t *acip = (ppc_cpuinfo_t *)cip->opaque;
  if (acip->frequency)
	return acip->frequency;

  return 0;
}

// Get processor socket ID
int cpuinfo_arch_get_socket(struct cpuinfo *cip)
{
  return -1;
}

// Get number of cores per CPU package
int cpuinfo_arch_get_cores(struct cpuinfo *cip)
{
  const ppc_spec_t *spec = get_ppc_spec(cip);
  if (spec)
	return spec->n_cores;

  return -1;
}

// Get number of threads per CPU core
int cpuinfo_arch_get_threads(struct cpuinfo *cip)
{
  const ppc_spec_t *spec = get_ppc_spec(cip);
  if (spec)
	return spec->n_threads;

  return -1;
}

// Get cache information
cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip)
{
  const ppc_spec_t *spec = get_ppc_spec(cip);
  if (spec) {
	cpuinfo_list_t caches_list = NULL;
	int i;
	for (i = 0; i < N_CACHE_DESCRIPTORS; i++) {
	  if (spec->caches[i])
		cpuinfo_caches_list_insert(spec->caches[i]);
	}
	return caches_list;
  }

  return NULL;
}

// Returns features table
uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature)
{
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	return cip->features;
  case CPUINFO_FEATURE_PPC:
	return ((ppc_cpuinfo_t *)(cip->opaque))->features;
  }
  return NULL;
}

// PPC features to check
static void check_hwcap_64bit(void)
{
  __asm__ __volatile__ (".long 0x7c000074"); // cntlzd r0,r0
}

static void check_hwcap_vmx(void)
{
  __asm__ __volatile__ (".long 0x10000484"); // vor v0,v0,v0
}

static void check_hwcap_fsqrt(void)
{
  __asm__ __volatile__ (".long 0xfc00002c"); // fsqrt f0,f0
}

// Returns 0 if CPU supports the specified feature
int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature)
{
  if (!cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_PPC)) {
	cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_PPC);

	if (cpuinfo_feature_test_function(check_hwcap_64bit))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_64BIT);

	if (cpuinfo_feature_test_function(check_hwcap_vmx))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_PPC_VMX);

	if (cpuinfo_feature_test_function(check_hwcap_fsqrt))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_PPC_FSQRT);

	if (cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_PPC_VMX))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_SIMD);
  }

  return cpuinfo_feature_get_bit(cip, feature);
}

#endif
