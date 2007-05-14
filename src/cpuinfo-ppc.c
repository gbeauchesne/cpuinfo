/*
 *  cpuinfo-ppc.c - Processor identification code, ppc specific
 *
 *  cpuinfo (C) 2006-2007 Gwenole Beauchesne
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "sysdeps.h"
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#if defined __APPLE__ && defined __MACH__
#include <mach-o/arch.h>
#endif

#define DEBUG 1
#include "debug.h"

// Extract information from Open Firmware
typedef char of_string_t[256];

enum {
  OF_TYPE_INVALID = -1,
  OF_TYPE_INT_32,
  OF_TYPE_INT_64,
  OF_TYPE_STRING,
};

typedef struct {
  const char *node;
  void *var;
  int type;
} of_property_t;

#define N_OF_PROPERTIES 10

typedef struct {
  int n_cpus;
  uint32_t cpu_version;
  uint32_t clock_frequency;
  uint32_t timebase_frequency;
  uint32_t d_cache_size;
  uint32_t d_cache_line_size;
  uint32_t i_cache_size;
  uint32_t i_cache_line_size;
  uint32_t l2cr;
  uint32_t l3cr;
  of_string_t name;
  // private data (for decoding & printing)
  of_property_t of_properties[N_OF_PROPERTIES + 1];
} of_info_t;

static int of_get_property(of_property_t *pnode, const char *path)
{
#if defined __APPLE__ && defined __MACH__
  switch (pnode->type) {
  case OF_TYPE_INT_32:
	*((uint32_t *)(pnode->var)) = strtoul(path, NULL, 16);
	break;
  case OF_TYPE_INT_64:
	*((uint64_t *)(pnode->var)) = strtoull(path, NULL, 16);
	break;
  case OF_TYPE_STRING: {
	char *str = (char *)pnode->var;
	if (sscanf(path, "\"%[^\"]", str) != 1)
	  strcpy(str, "<unknown>");
	break;
  }
  }
  return 0;
#elif defined __linux__
  FILE *fp = fopen(path, "r");
  if (fp) {
	switch (pnode->type) {
	case OF_TYPE_INT_32: {
	  uint32_t value;
	  if (fread(&value, sizeof(value), 1, fp) != 1)
		value = 0;
	  *((uint32_t *)(pnode->var)) = value;
	  break;
	}
	case OF_TYPE_INT_64: {
	  uint64_t value;
	  if (fread(&value, sizeof(value), 1, fp) != 1)
		value = 0;
	  *((uint64_t *)(pnode->var)) = value;
	  break;
	}
	case OF_TYPE_STRING: {
	  char *str = (char *)pnode->var;
	  if (fgets(str, sizeof(of_string_t), fp) == NULL)
		strcpy(str, "<unknown>");
	  break;
	}
	}
	fclose(fp);
	return 0;
  }
#endif
  return -1;
}

static int of_get_properties(of_info_t *ofip)
{
  if (ofip == NULL)
	return -1;

  memset(ofip, 0, sizeof(*ofip));
#define OF_PROP_INIT(ID, NAME, VAR, TYPE) do {		\
	ofip->of_properties[ID].node = NAME;			\
	ofip->of_properties[ID].var = &ofip->VAR;		\
	ofip->of_properties[ID].type = OF_TYPE_##TYPE;	\
  } while (0)
  OF_PROP_INIT(0, "clock-frequency",		clock_frequency,	INT_32);
  OF_PROP_INIT(1, "timebase-frequency",		timebase_frequency,	INT_32);
  OF_PROP_INIT(2, "d-cache-size",			d_cache_size,		INT_32);
  OF_PROP_INIT(3, "d-cache-line-size",		d_cache_line_size,	INT_32);
  OF_PROP_INIT(4, "i-cache-size",			i_cache_size,		INT_32);
  OF_PROP_INIT(5, "i-cache-line-size",		i_cache_line_size,	INT_32);
  OF_PROP_INIT(6, "l2cr",					l2cr,				INT_32);
  OF_PROP_INIT(7, "l3cr",					l3cr,				INT_32);
  OF_PROP_INIT(8, "name",					name,				STRING);
  OF_PROP_INIT(9, "cpu-version",			cpu_version,		INT_32);
#undef OF_PROP_INIT

  int i;
  int n_cpus = 0;
#if defined __APPLE__ && defined __MACH__
  FILE *proc_file = popen("ioreg -c IOPlatformDevice", "r");
  if (proc_file == NULL)
	return NULL;
  char line[256];
  int powerpc_node = 0;
  while (fgets(line, sizeof(line), proc_file)) {
	// Read line
	int len = strlen(line);
	if (len == 0)
	  continue;
	line[len - 1] = 0;

	// Parse line
	if (strstr(line, "o PowerPC,")) {
	  powerpc_node = 1;
	  ++n_cpus;
	}
	else if (powerpc_node && n_cpus == 1) {
	  char key[256], value[256], dummy[256];
	  if (sscanf(line, "%[ |]\"%[^\"]\" = <%[^>]>", dummy, key, value) == 3) {
		for (i = 0; ofip->of_properties[i].node != NULL; i++) {
		  of_property_t *pnode = &ofip->of_properties[i];
		  if (strcmp(pnode->node, key) == 0) {
			if (of_get_property(pnode, value) < 0)
			  D(bug("failed to read property %s, value %s\n", key, value));
		  }
		}
	  }
	  else if (strchr(line, '}'))
		powerpc_node = 0;
	}
  }
  fclose(proc_file);
#elif defined __linux__
  char path[PATH_MAX];
  struct dirent *de;
  static const char oftree_cpus[] = "/proc/device-tree/cpus";
  DIR *d = opendir(oftree_cpus);
  if (d == NULL)
	return -1;
  while ((de = readdir(d)) != NULL) {
	if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	  continue;
	struct stat st;
	int ret = snprintf(path, sizeof(path), "%s/%s", oftree_cpus, de->d_name);
	if (ret < 0 || ret >= sizeof(path))
	  continue;
	stat(path, &st);
	if (!S_ISDIR(st.st_mode))
	  continue;
	++n_cpus;
	if (n_cpus == 1) {
	  for (i = 0; ofip->of_properties[i].node != NULL; i++) {
		of_property_t *pnode = &ofip->of_properties[i];
		int ret = snprintf(path, sizeof(path), "%s/%s/%s", oftree_cpus, de->d_name, pnode->node);
		if (ret < 0 || ret >= sizeof(path))
		  continue;
		if (of_get_property(pnode, path) < 0)
		  D(bug("failed to read property %s\n", path));
	  }
	}
  }
#else
  return -1;
#endif
  ofip->n_cpus = n_cpus;
  return 0;
}

// Arch-dependent data
struct ppc_cpuinfo {
  uint32_t pvr;
  uint32_t l2cr;
  uint32_t l3cr;
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
DEFINE_CACHE_DESCRIPTOR(L1_16KB,	UNIFIED,	1,	   16);
DEFINE_CACHE_DESCRIPTOR(L1_32KB,	UNIFIED,	1,	   32);
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
	{ &L1_32KB }
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
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 7400 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7400EC.pdf> */
	0xffff0000, 0x000c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7400",
	1, 1,
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 7410 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7410EC.pdf> */
	0xffff0000, 0x800c0000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7410",
	1, 1,
	{ &L1I_32KB, &L1D_32KB }
  },
  { /* PowerPC 7450 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7450EC.pdf> */
	0xffff0000, 0x80000000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7450",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 7455 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7455EC.pdf> */
	0xffff0000, 0x80010000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7455",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_256KB }
  },
  { /* PowerPC 7457 */
	/* <http://www.freescale.com/files/32bit/doc/data_sheet/MPC7457EC.pdf> */
	0xffff0000, 0x80020000,
	CPUINFO_VENDOR_MOTOROLA, "PowerPC 7457",
	1, 1,
	{ &L1I_32KB, &L1D_32KB, &L2_512KB }
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
	CPUINFO_VENDOR_IBM, "POWER5",
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
  PVR_POWERPC_604EV		= 0x000a0000,
  PVR_POWERPC_750		= 0x00080000,
  PVR_POWERPC_750FX		= 0x70000000,
  PVR_POWERPC_750FL		= 0x700a0000,
  PVR_POWERPC_750GX		= 0x70020000,
  PVR_POWERPC_7400		= 0x000c0000,
  PVR_POWERPC_7410		= 0x800c0000,
  PVR_POWERPC_7450		= 0x80000000,
  PVR_POWERPC_7455		= 0x80010000,
  PVR_POWERPC_7457		= 0x80020000,
  PVR_POWERPC_7447A		= 0x80030000,
  PVR_POWERPC_7448		= 0x80040000,
  PVR_POWERPC_970		= 0x00390000,
  PVR_POWERPC_970FX		= 0x003c0000,
  PVR_POWERPC_970MP		= 0x00440000,
  PVR_POWER3			= 0x00400000,
  PVR_POWER3PLUS		= 0x00410000,
  PVR_POWER4			= 0x00350000,
  PVR_POWER4PLUS		= 0x00380000,
  PVR_POWER5			= 0x003a0000,
  PVR_POWER5PLUS		= 0x003b0000,
  PVR_POWER6			= 0x003e0000,
  PVR_CELL				= 0x00700000,
};

#define PVR_MATCH(PVR, VALUE)	(((PVR) & 0xffff0000) == (VALUE))
#define PVR_POWERPC(PVR, MODEL)	PVR_MATCH(PVR, PVR_POWERPC_##MODEL)
#define IS_POWERPC_601(PVR)		(PVR_POWERPC(PVR, 601))
#define IS_POWERPC_603(PVR)		(PVR_POWERPC(PVR, 603) || PVR_POWERPC(PVR, 603E) || PVR_POWERPC(PVR, 603EV))
#define IS_POWERPC_604(PVR)		(PVR_POWERPC(PVR, 604) || PVR_POWERPC(PVR, 604E) || PVR_POWERPC(PVR, 604EV))
#define IS_POWERPC_7400(PVR)	(PVR_POWERPC(PVR, 7400) || PVR_POWERPC(PVR, 7410))
#define IS_POWERPC_745X(PVR)	(PVR_POWERPC(PVR, 7450) || PVR_POWERPC(PVR, 7455) || PVR_POWERPC(PVR, 7457) || PVR_POWERPC(PVR, 7447A) || PVR_POWERPC(PVR, 7448))
#define IS_POWERPC_970(PVR)		(PVR_POWERPC(PVR, 970) || PVR_POWERPC(PVR, 970FX) || PVR_POWERPC(PVR, 970MP))

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
  acip->l2cr = 0;
  acip->l3cr = 0;
  acip->frequency = 0;
  memset(acip->features, 0, sizeof(acip->features));

  if (cpuinfo_feature_test_function((cpuinfo_feature_test_function_t)get_pvr))
	acip->pvr = get_pvr();

  of_info_t of_info;
  if (of_get_properties(&of_info) == 0) {
	acip->l2cr = of_info.l2cr;
	acip->l3cr = of_info.l3cr;
	acip->frequency = of_info.clock_frequency / (1000 * 1000);
	if (acip->pvr == 0 && of_info.cpu_version != 0)
	  acip->pvr = of_info.cpu_version;
  }

#if defined __APPLE__ && defined __MACH__
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
  if (acip->frequency == 0) {
	FILE *proc_file = fopen("/proc/cpuinfo", "r");
	if (proc_file) {
	  char line[256];
	  while(fgets(line, sizeof(line), proc_file)) {
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
  }
#endif

  if (acip->pvr == 0)
	return -1;

  return 0;
}

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  ppc_cpuinfo_t *p = (ppc_cpuinfo_t *)malloc(sizeof(*p));
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

// Dump all useful information for debugging
int cpuinfo_dump(struct cpuinfo *cip, FILE *out)
{
  if (cip == NULL)
	return -1;

  ppc_cpuinfo_t *acip = (ppc_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return -1;

  of_info_t of_info;
  if (of_get_properties(&of_info) < 0)
	return -1;

  of_info_t *ofip = &of_info;
  fprintf(out, "System with %d CPUs\n", ofip->n_cpus);
  fprintf(out, "\n");

  int i;
  fprintf(out, "%-30s %08x\n", "pvr", acip->pvr);
  for (i = 0; ofip->of_properties[i].node != NULL; i++) {
	of_property_t *pnode = &ofip->of_properties[i];
	switch (pnode->type) {
	case OF_TYPE_INT_32:
	  fprintf(out, "%-30s %08x\n", pnode->node, *((uint32_t *)(pnode->var)));
	  break;
	case OF_TYPE_INT_64:
	  fprintf(out, "%-30s %" PRIx64 "\n", pnode->node, *((uint64_t *)(pnode->var)));
	  break;
	case OF_TYPE_STRING:
	  fprintf(out, "%-30s '%s'\n", pnode->node, (char *)pnode->var);
	  break;
	}
  }

  return 0;
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
	char *model = (char *)malloc(strlen(spec->model) + 1);
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

// Decode L2 Control Register
static int decode_l2cr(struct cpuinfo *cip, cpuinfo_cache_descriptor_t *cdp)
{
  if (cip == NULL || cdp == NULL)
	return -1;

  ppc_cpuinfo_t *acip = (ppc_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return -1;

  uint32_t l2cr = acip->l2cr;
  if (l2cr == 0 || (l2cr & 0x80000000) == 0)
	return -1;

  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_MOTOROLA) {
	cdp->level = 2;
	cdp->type = CPUINFO_CACHE_TYPE_UNIFIED; // XXX check L2IO/L2DO?
	switch ((l2cr >> 28) & 3) {
	case 0: cdp->size = 2048;	break;
	case 1:	cdp->size = 256;	break;
	case 2: cdp->size = 512;	break;
	case 3: cdp->size = 1024;	break;
	}
	return 0;
  }
  
  return -1;
}

// Decode L3 Control Register
static int decode_l3cr(struct cpuinfo *cip, cpuinfo_cache_descriptor_t *cdp)
{
  if (cip == NULL || cip->opaque == NULL || cdp == NULL)
	return -1;

  ppc_cpuinfo_t *acip = (ppc_cpuinfo_t *)(cip->opaque);
  if (acip == NULL)
	return -1;

  uint32_t l3cr = acip->l3cr;
  if (l3cr == 0 || (l3cr & 0x80000000) == 0)
	return -1;

  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_MOTOROLA) {
	cdp->level = 3;
	cdp->type = CPUINFO_CACHE_TYPE_UNIFIED; // XXX check L3IO/L3DO?
	cdp->size = ((l3cr >> 28) & 1) ? 2048 : 1024;
	return 0;
  }

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

	cpuinfo_cache_descriptor_t cache_desc;
	if (decode_l2cr(cip, &cache_desc) == 0)
	  cpuinfo_caches_list_insert(&cache_desc);
	if (decode_l3cr(cip, &cache_desc) == 0)
	  cpuinfo_caches_list_insert(&cache_desc);

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

// Returns 1 if CPU supports the specified feature
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
