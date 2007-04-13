/*
 *  cpuinfo-ppc.c - Processor identification code, ppc specific
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
DEFINE_CACHE_DESCRIPTOR(L1I_8KB,	CODE,		1,	    8);
DEFINE_CACHE_DESCRIPTOR(L1I_16KB,	CODE,		1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1I_32KB,	CODE,		1,	   32);
DEFINE_CACHE_DESCRIPTOR(L1I_64KB,	CODE,		1,	   64);
DEFINE_CACHE_DESCRIPTOR(L1D_8KB,	DATA,		1,	    8);
DEFINE_CACHE_DESCRIPTOR(L1D_16KB,	DATA,		1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1D_32KB,	DATA,		1,	   32);
DEFINE_CACHE_DESCRIPTOR(L1D_64KB,	DATA,		1,	   64);
DEFINE_CACHE_DESCRIPTOR(L1U_16KB,	UNIFIED,	1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1U_32KB,	UNIFIED,	1,	   32);
DEFINE_CACHE_DESCRIPTOR(L2_256KB,	UNIFIED,	2,	  256);
DEFINE_CACHE_DESCRIPTOR(L2_512KB,	UNIFIED,	2,	  512);
DEFINE_CACHE_DESCRIPTOR(L2_1440KB,	UNIFIED,	2,	 1440);
DEFINE_CACHE_DESCRIPTOR(L2_1920KB,	UNIFIED,	2,	 1920);
DEFINE_CACHE_DESCRIPTOR(L2_1MB,		UNIFIED,	2,	 1024);
DEFINE_CACHE_DESCRIPTOR(L2_2MB,		UNIFIED,	2,	 2048);
DEFINE_CACHE_DESCRIPTOR(L2_4MB,		UNIFIED,	2,	 4096);
DEFINE_CACHE_DESCRIPTOR(L3_32MB,	UNIFIED,	3,	32768);
DEFINE_CACHE_DESCRIPTOR(L3_36MB,	UNIFIED,	3,	36864);
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
  { /* PowerPC 601 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC601.pdf> */
	0xffff0000, 0x00010000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 601",
	1, 1,
	{ &L1U_32KB }
  },
  { /* PowerPC 603 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC603.pdf> */
	0xffff0000, 0x00030000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603",
	1, 1,
	{ &L1I_8KB, &L1D_8KB }
  },
  { /* PowerPC 603e */
	/* <http://www.freescale.com/files/32bit/doc/prod_brief/MPC603E.pdf> */
	0xffff0000, 0x00060000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603e",
	1, 1,
	{ &L1I_16KB, &L1D_16KB }
  },
  { /* PowerPC 603ev */
	0xffff0000, 0x00070000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 603ev",
	1, 1,
	{ &L1I_16KB, &L1D_16KB }
  },
  { /* PowerPC 604 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC604.pdf> */
	0xffff0000, 0x00040000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604",
	1, 1,
	{ &L1I_16KB, &L1D_16KB }
  },
  { /* PowerPC 604e */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC604E.pdf> */
	0xfffff000, 0x00090000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604e",
	1, 1,
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 604r */
	0xffff0000, 0x00090000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604r",
	1, 1,
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 604ev */
	0xffff0000, 0x000a0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 604ev",
	1, 1,
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 750CX */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/220134650EDFEB9187256AE8006CF163/$file/sw_ds_general.pdf> */
	0xfffffff0, 0x00080100,
	CPUINFO_VENDOR_IBM, "PowerPC 750CX",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 750CX */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/220134650EDFEB9187256AE8006CF163/$file/sw_ds_general.pdf> */
	0xfffffff0, 0x00082200,
	CPUINFO_VENDOR_IBM, "PowerPC 750CX",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 750CXe */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/31777DF6FD56656387256B1B0076E671/$file/750cxedd3.1_ds.pdf> */
	0xfffffff0, 0x00082210,
	CPUINFO_VENDOR_IBM, "PowerPC 750CXe",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 750CXr */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/A07229E92706089A87256DCD005BFBE9/$file/sw_ds_750cxr2-28-05.pdf> */
	0xfffffff0, 0x00083410,
	CPUINFO_VENDOR_IBM, "PowerPC 750CXr",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 750FX */
	0xffff0000, 0x70000000,
	CPUINFO_VENDOR_IBM, "PowerPC 750FX",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
  },
  { /* PowerPC 750FL */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/ED29A8828F4E10AB87256FDA00742D1D/$file/750FL_DS_6-22-06.pdf> */
	0xffffffff, 0x700a02b3,
	CPUINFO_VENDOR_IBM, "PowerPC 750FL",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
  },
  { /* PowerPC 750FX */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/A571994FDFA77C3287256C31004AF7CD/$file/750fxdd2_ds.pdf> */
	0xffffff00, 0x700a0200,
	CPUINFO_VENDOR_IBM, "PowerPC 750FX",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
  },
  { /* PowerPC 750GL */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/6C89C8231496F8198725700E00776940/$file/750GL_ds3-13-06.pdf> */
	0xffffff0f, 0x70020102,
	CPUINFO_VENDOR_IBM, "PowerPC 750GL",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_1MB }
  },
  { /* PowerPC 750GX */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/4D86B2273E8218CE87256E660058763D/$file/750GX_ds9-2-05.pdf> */
	0xffff0000, 0x70020000,
	CPUINFO_VENDOR_IBM, "PowerPC 750GX",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_1MB }
  },
  { /* PowerPC 750 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC750EC.pdf> */
	0xffff0000, 0x00080000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 750",
	1, 1,
	{ &L1I_32KB, &L1D_32KB } /* XXX: external L2 cache, check L2CR? */
  },
  { /* PowerPC 7400 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7400EC.pdf> */
	0xffff0000, 0x000c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7400",
	1, 1,
	{ &L1I_32KB, &L1D_32KB } /* XXX: external L2 cache, check L2CR? */
  },
  { /* PowerPC 7410 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7410EC.pdf> */
	0xffff0000, 0x800c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7410",
	1, 1,
	{ &L1I_32KB, &L1D_32KB } /* XXX: external L2 cache, check L2CR? */
  },
  { /* PowerPC 7450 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7450EC.pdf> */
	0xffff0000, 0x80000000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7450",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB } /* XXX: external L3 cache, check L3CR? */
  },
  { /* PowerPC 7455 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7455EC.pdf> */
	0xffff0000, 0x80010000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7455",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB } /* XXX: external L3 cache, check L3CR? */
  },
  { /* PowerPC 7457 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7457EC.pdf> */
	0xffff0000, 0x80020000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7457",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB } /* XXX: external L3 cache, check L3CR? */
  },
  { /* PowerPC 7447A */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7447AEC.pdf> */
	0xffff0000, 0x80030000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7447A",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
  },
  { /* PowerPC 7448 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7448EC.pdf> */
	0xffff0000, 0x80040000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7448",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_1MB }
  },
  {
	/* PowerPC 970 */
	0xffff0000, 0x00390000,
	CPUINFO_VENDOR_IBM, "PowerPC 970",
	1, 1,
	{ &L1I_64KB, &L1D_32KB, &L2_512KB }
  },
  {
	/* PowerPC 970FX */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/1DE505664D202D2987256D9C006B90A5/$file/PPC970FX_DS_DD3.X_V2.5_26MAR2007_pub.pdf> */
	0xffff0000, 0x003c0000,
	CPUINFO_VENDOR_IBM, "PowerPC 970FX",
	1, 1,
	{ &L1I_64KB, &L1D_32KB, &L2_512KB }
  },
  {
	/* PowerPC 970MP */
	/* <http://www-306.ibm.com/chips/techlib/techlib.nsf/techdocs/B9C08F2F7CF5709587256F8C006727F1/$file/970MP_DS_DD1.1x_v1.1_pub_29Mar2007.pdf> */
	0xffff0000, 0x00440000,
	CPUINFO_VENDOR_IBM, "PowerPC 970MP",
	2, 1,
	{ &L1I_64KB, &L1D_32KB, &L2_1MB }
  },
  {
	/* POWER3 */
	/* <http://www.redbooks.ibm.com/redbooks/pdfs/sg245155.pdf> */
	0xffff0000, 0x00400000,
	CPUINFO_VENDOR_IBM, "POWER3",
	1, 1,
	{ &L1I_32KB, &L1D_64KB } /* XXX: external L2 cache (1-16 MB), check L2CR? */
  },
  {
	/* POWER3+ */
	0xffff0000, 0x00410000,
	CPUINFO_VENDOR_IBM, "POWER3+",
	1, 1,
	{ &L1I_32KB, &L1D_64KB } /* XXX: external L2 cache (1-16 MB), check L2CR? */
  },
  {
	/* POWER4 */
	/* <http://www.redbooks.ibm.com/redbooks/pdfs/sg247041.pdf> */
	0xffff0000, 0x00350000,
	CPUINFO_VENDOR_IBM, "POWER4",
	2, 1,
	{ &L1I_64KB, &L1D_32KB, &L2_1440KB, &L3_32MB } /* XXX: up to 128 MB shared on the same MCM */
  },
  {
	/* POWER4+ */
	0xffff0000, 0x00380000,
	CPUINFO_VENDOR_IBM, "POWER4+",
	2, 1,
	{ &L1I_64KB, &L1D_32KB, &L2_1440KB, &L3_32MB } /* XXX: up to 128 MB shared on the same MCM */
  },
  {
	/* POWER5 */
	0xffff0000, 0x003a0000,
	CPUINFO_VENDOR_IBM, "POWER4",
	2, 2,
	{ &L1I_64KB, &L1D_32KB, &L2_1920KB, &L3_36MB }
  },
  {
	/* POWER5+ */
	0xffff0000, 0x003b0000,
	CPUINFO_VENDOR_IBM, "POWER5+",
	2, 2,
	{ &L1I_64KB, &L1D_32KB, &L2_1920KB, &L3_36MB }
  },
  {
	/* POWER6 */
	0xffff0000, 0x003e0000,
	CPUINFO_VENDOR_IBM, "POWER6",
	2, 2,
	{ &L1I_64KB, &L1D_64KB, &L2_4MB, &L3_32MB }
  },
  {
	/* Cell PPE */
	0xffff0000, 0x00700000,
	CPUINFO_VENDOR_IBM, "Cell",
	1, 2,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
  },
  { /* Unknown */
	0x00000000, 0x00000000,
	CPUINFO_VENDOR_UNKNOWN, NULL,
	-1, -1,
	{ NULL }
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
