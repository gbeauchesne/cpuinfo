/*
 *  cpuinfo-mips.c - Processor identification code, mips specific
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

#if defined __sgi
#include <invent.h>
#include <sys/sbd.h>
#endif

#define DEBUG 1
#include "debug.h"

// CPU caches specifications
#define DEFINE_CACHE_DESCRIPTOR(NAME, TYPE, LEVEL, SIZE) \
static const cpuinfo_cache_descriptor_t NAME = { CPUINFO_CACHE_TYPE_##TYPE, LEVEL, SIZE }
DEFINE_CACHE_DESCRIPTOR(L1I_8KB,	CODE,		1,	    8);
DEFINE_CACHE_DESCRIPTOR(L1I_16KB,	CODE,		1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1I_32KB,	CODE,		1,	   32);
DEFINE_CACHE_DESCRIPTOR(L1D_8KB,	DATA,		1,	    8);
DEFINE_CACHE_DESCRIPTOR(L1D_16KB,	DATA,		1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1D_32KB,	DATA,		1,	   32);
DEFINE_CACHE_DESCRIPTOR(L2_512KB,	UNIFIED,	2,	  512);
#undef DEFINE_CACHE_DESCRIPTOR

// CPU specs table
#define N_CACHE_DESCRIPTORS 3
struct mips_spec_entry {
  uint32_t prid_mask;
  uint32_t prid_value;
  int vendor;
  char *model;
  const cpuinfo_cache_descriptor_t *caches[N_CACHE_DESCRIPTORS];
};

typedef struct mips_spec_entry mips_spec_t;

static const mips_spec_t mips_specs[] = {
  { /* MIPS R16000 */
	0xfff0, 0x0f30,
	CPUINFO_VENDOR_MIPS, "R16000",
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* MIPS R14000 */
	0xff00, 0x0f00,
	CPUINFO_VENDOR_MIPS, "R14000",
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* MIPS R12000 */
	/* <http://sc.tamu.edu/help/power/powerlearn/reference/R12000_developer.ps> */
	0xff00, 0x0e00,
	CPUINFO_VENDOR_MIPS, "R12000",
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* MIPS R10000 */
	/* <http://techpubs.sgi.com/library/manuals/2000/007-2490-001/pdf/007-2490-001.pdf> */
	0xff00, 0x0900,
	CPUINFO_VENDOR_MIPS, "R10000",
	{ &L1I_32KB, &L1D_32KB } /* XXX: external L2 cache (512 KB to 16 MB) */
  },
  { /* MIPS R8000 */
	0xff00, 0x1000,
	CPUINFO_VENDOR_MIPS, "R8000",
	{ NULL }
  },
  { /* PMC-SIERRA RM7000 */
	/* <http://www.pmc-sierra.com/products/details/rm7000/> */
	0xff00, 0x2700,
	CPUINFO_VENDOR_PMC, "RM7000",
	{ &L1I_16KB, &L1D_16KB, &L2_512KB } /* XXX: external L3 cache (up to 64 MB) */
  },
  { /* MIPS R6000A */
	0xff00, 0x0600,
	CPUINFO_VENDOR_MIPS, "R6000A",
	{ NULL }
  },
  { /* MIPS R6000 */
	/* <http://www.linux-mips.org/wiki/R6000> */
	0xff00, 0x0300,
	CPUINFO_VENDOR_MIPS, "R6000",
	{ &L1I_16KB, &L1D_16KB, &L2_512KB } /* XXX: L1 I-cache can be increased to 64KB */
  },
  { /* PMC-SIERRA R5271 */
	0xff00, 0x2800,
	CPUINFO_VENDOR_PMC, "RM5271",
	{ NULL }
  },
  { /* MIPS R5000 */
	/* <http://www.mips.com/content/Documentation/MIPSDocumentation/RSeriesDocs/content_html/documents/R5000%20Product%20Information.pdf> */
	0xff00, 0x2300,
	CPUINFO_VENDOR_MIPS, "R5000",
	{ &L1I_32KB, &L1D_32KB } /* XXX: external L2 cache (512 KB to 2 MB) */
  },
  { /* MIPS R4700 */
	0xff00, 0x2100,
	CPUINFO_VENDOR_MIPS, "R4700",
	{ NULL }
  },
  { /* MIPS R4650 */
	0xff00, 0x2200,
	CPUINFO_VENDOR_MIPS, "R4650",
	{ NULL }
  },
  { /* MIPS R4600 */
	0xff00, 0x2000,
	CPUINFO_VENDOR_MIPS, "R4600",
	{ &L1I_16KB, &L1D_16KB }
  },
  { /* MIPS R4400 */
	0xfff0, 0x0440,
	CPUINFO_VENDOR_MIPS, "R4400",
	{ &L1I_16KB, &L1D_16KB } /* XXX: external L2 cache (128 KB to 4 MB) */
  },
  { /* MIPS R4300i */
	/* <http://www.mips.com/content/Documentation/MIPSDocumentation/RSeriesDocs/content_html/documents/R4300i%20Product%20Information.pdf> */
	0xff00, 0x0b00,
	CPUINFO_VENDOR_MIPS, "R4300i",
	{ &L1I_16KB, &L1D_8KB }
  },
  { /* MIPS R4000 */
	/* <http://www.mips.com/Documentation/MIPSDocumentation/RSeriesDocs/content_html/documents/R4000%20Microprocessor%20Users%20Manual.pdf> */
	0xff00, 0x0400,
	CPUINFO_VENDOR_MIPS, "R4000",
	{ &L1I_8KB, &L1D_8KB } /* XXX: up to 32 KB L1 caches, external L2 cache (128 KB to 4 MB) */
  },
  { /* MIPS R3000A */
	0xfff0, 0x0220,
	CPUINFO_VENDOR_MIPS, "R3000A",
	{ NULL }
  },
  { /* MIPS R3000 */
	0xfff0, 0x0210,
	CPUINFO_VENDOR_MIPS, "R3000",
	{ NULL }
  },
  { /* MIPS R2000A */
	0xff00, 0x0200,
	CPUINFO_VENDOR_MIPS, "R2000A",
	{ NULL }
  },
  { /* MIPS R2000A */
	0xfff0, 0x0110,
	CPUINFO_VENDOR_MIPS, "R2000A",
	{ NULL }
  },
  { /* MIPS R2000 */
	0xff00, 0x0100,
	CPUINFO_VENDOR_MIPS, "R2000",
	{ NULL }
  },
  { /* Unknown */
	0x0000, 0x0000,
	CPUINFO_VENDOR_UNKNOWN, NULL,
	{ NULL }
  }
};

// Lookup CPU spec
static const mips_spec_t *lookup_mips_spec(uint32_t prid)
{
  const mips_spec_t *spec = NULL;
  const int n_specs = sizeof(mips_specs) / sizeof(mips_specs[0]);
  for (int i = 0; i < n_specs; i++) {
	if ((prid & mips_specs[i].prid_mask) == mips_specs[i].prid_value) {
	  spec = &mips_specs[i];
	  break;
	}
  }
  assert(spec != NULL);
  return spec;
}

// Arch-dependent data
struct mips_cpuinfo {
  uint32_t prid;
  uint32_t frequency;
  int vendor;
  const char *model;
  cpuinfo_list_t caches;
  uint32_t features[CPUINFO_FEATURES_SZ_(MIPS)];
};

typedef struct mips_cpuinfo mips_cpuinfo_t;

// Initialize arch-dependent cpuinfo datastructure
static int cpuinfo_arch_init(mips_cpuinfo_t *acip)
{
  acip->prid = 0;
  acip->frequency = 0;
  acip->vendor = CPUINFO_VENDOR_UNKNOWN;
  acip->model = NULL;
  acip->caches = NULL;
  memset(&acip->features, 0, sizeof(acip->features));

  cpuinfo_list_t caches_list = NULL;
#if defined __sgi
  inv_state_t *isp = NULL;
  cpuinfo_cache_descriptor_t cache_desc;
  if (setinvent_r(&isp) < 0)
	return -1;
  inventory_t *inv;
  while ((inv = getinvent_r(isp)) != NULL) {
	switch (inv->inv_class) {
	case INV_PROCESSOR:
	  switch (inv->inv_type) {
	  case INV_CPUBOARD:
		acip->frequency = inv->inv_controller;
		break;
	  case INV_CPUCHIP:
		acip->prid = inv->inv_state;
		break;
	  }
	  break;
	case INV_MEMORY:
	  switch (inv->inv_type) {
	  case INV_ICACHE:
		cache_desc.level = 1;
		cache_desc.type = CPUINFO_CACHE_TYPE_CODE;
		cache_desc.size = inv->inv_state / 1024;
		cpuinfo_list_insert(&caches_list, &cache_desc);
		break;
	  case INV_DCACHE:
		cache_desc.level = 1;
		cache_desc.type = CPUINFO_CACHE_TYPE_DATA;
		cache_desc.size = inv->inv_state / 1024;
		cpuinfo_list_insert(&caches_list, &cache_desc);
		break;
	  case INV_SICACHE:
		cache_desc.level = 2;
		cache_desc.type = CPUINFO_CACHE_TYPE_CODE;
		cache_desc.size = inv->inv_state / 1024;
		cpuinfo_list_insert(&caches_list, &cache_desc);
		break;
	  case INV_SDCACHE:
		cache_desc.level = 2;
		cache_desc.type = CPUINFO_CACHE_TYPE_DATA;
		cache_desc.size = inv->inv_state / 1024;
		cpuinfo_list_insert(&caches_list, &cache_desc);
		break;
	  case INV_SIDCACHE:
		cache_desc.level = 2;
		cache_desc.type = CPUINFO_CACHE_TYPE_UNIFIED;
		cache_desc.size = inv->inv_state / 1024;
		cpuinfo_list_insert(&caches_list, &cache_desc);
		break;
	  }
	  break;
	}
  }
  endinvent_r(isp);
#endif

  if (acip->prid == 0)
	return -1;

  const mips_spec_t *spec = lookup_mips_spec(acip->prid);
  if (spec) {
	acip->vendor = spec->vendor;
	acip->model = spec->model;
	if (caches_list == NULL) {
	  for (int i = 0; i < N_CACHE_DESCRIPTORS; i++) {
		if (spec->caches[i]) {
		  if (cpuinfo_list_insert(&caches_list, spec->caches[i]) < 0) {
			cpuinfo_list_clear(&caches_list);
			return -1;
		  }
		}
	  }
	}
  }

  acip->caches = caches_list;

  // XXX: fill in additional vendors

  return 0;
}

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  mips_cpuinfo_t *p = (mips_cpuinfo_t *)malloc(sizeof(*p));
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
  mips_cpuinfo_t *acip = (mips_cpuinfo_t *)cip->opaque;
  if (acip)
	free(acip);
}

// Dump all useful information for debugging
int cpuinfo_dump(struct cpuinfo *cip, FILE *out)
{
  if (cip == NULL)
	return -1;

  mips_cpuinfo_t *acip = (mips_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return -1;

  fprintf(out, "prid %08x\n", acip->prid);
  return 0;
}

// Get processor vendor ID 
int cpuinfo_arch_get_vendor(struct cpuinfo *cip)
{
  return ((mips_cpuinfo_t *)(cip->opaque))->vendor;
}

// Get processor name
char *cpuinfo_arch_get_model(struct cpuinfo *cip)
{
  const char *imodel = ((mips_cpuinfo_t *)(cip->opaque))->model;
  if (imodel) {
	char *model = (char *)malloc(strlen(imodel) + 1);
	if (model)
	  strcpy(model, imodel);
	return model;
  }
  return NULL;
}

// Get processor frequency in MHz
int cpuinfo_arch_get_frequency(struct cpuinfo *cip)
{
  return ((mips_cpuinfo_t *)(cip->opaque))->frequency;
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
  return ((mips_cpuinfo_t *)(cip->opaque))->caches;
}

// Returns features table
uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature)
{
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	return cip->features;
  case CPUINFO_FEATURE_MIPS:
	return ((mips_cpuinfo_t *)(cip->opaque))->features;
  }
  return NULL;
}

// Returns 0 if CPU supports the specified feature
int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature)
{
  if (!cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_MIPS)) {
	cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_MIPS);
  }

  return cpuinfo_feature_get_bit(cip, feature);
}
