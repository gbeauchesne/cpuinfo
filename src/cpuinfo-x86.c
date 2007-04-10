/*
 *  cpuinfo-x86.c - Processor identification code, x86 specific
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
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#define DEBUG 1
#include "debug.h"

#if defined __i386__ || defined __x86_64__

static void cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  uint32_t a = eax ? *eax : 0;
  uint32_t b = ebx ? *ebx : 0;
  uint32_t c = ecx ? *ecx : 0;
  uint32_t d = edx ? *edx : 0;

#if defined __i386__
  __asm__ __volatile__ ("xchgl	%%ebx,%0\n\t"
						"cpuid	\n\t"
						"xchgl	%%ebx,%0\n\t"
						: "+r" (b), "=a" (a), "=c" (c), "=d" (d)
						: "1" (op), "2" (c));
#else
  __asm__ __volatile__ ("cpuid"
						: "=a" (a), "=b" (b), "=c" (c), "=d" (d)
						: "0" (op), "2" (c));
#endif

  if (eax) *eax = a;
  if (ebx) *ebx = b;
  if (ecx) *ecx = c;
  if (edx) *edx = d;
}

// Arch-dependent data
struct x86_cpuinfo {
  uint32_t features[CPUINFO_FEATURES_SZ_(X86)];
};

typedef struct x86_cpuinfo x86_cpuinfo_t;

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  x86_cpuinfo_t *p = malloc(sizeof(*p));
  if (p == NULL)
	return -1;
  memset(p->features, 0, sizeof(p->features));
  cip->opaque = p;
  return 0;
}

// Release the cpuinfo descriptor and all allocated data
void cpuinfo_arch_destroy(struct cpuinfo *cip)
{
  if (cip->opaque)
	free(cip->opaque);
}

// Get processor vendor ID 
int cpuinfo_arch_get_vendor(struct cpuinfo *cip)
{
  int vendor;

  char v[13] = { 0, };
  cpuid(0, NULL, (uint32_t *)&v[0], (uint32_t *)&v[8], (uint32_t *)&v[4]);

  if (!strcmp(v, "GenuineIntel"))
	vendor = CPUINFO_VENDOR_INTEL;
  else if (!strcmp(v, "AuthenticAMD"))
	vendor = CPUINFO_VENDOR_AMD;
  else if (!strcmp(v, "GenuineTMx86") || !strcmp(v, "TransmetaCPU"))
	vendor = CPUINFO_VENDOR_TRANSMETA;
  else
	vendor = CPUINFO_VENDOR_UNKNOWN;

  return vendor;
}

// Get AMD processor name
static char *get_model_amd_npt(struct cpuinfo *cip)
{
  // assume we are a valid AMD NPT Family 0Fh processor
  uint32_t eax, ebx;
  cpuid(0x80000001, &eax, &ebx, NULL, NULL);
  uint32_t BrandId = ebx & 0xffff;

  uint32_t PwrLmt = ((BrandId >> 5) & 0xe) | ((BrandId >> 14) & 1);		// BrandId[8:6,14]
  uint32_t BrandTableIndex = (BrandId >> 9) & 0x1f;						// BrandId[13:9]
  uint32_t NN = ((BrandId >> 9) & 0x40) | (BrandId & 0x3f);				// BrandId[15,5:0]
  uint32_t CmpCap = cpuinfo_get_cores(cip) > 1;

  typedef struct processor_name_string {
	int8_t cmp;
	uint8_t index;
	uint8_t pwr_lmt;
	const char *name;
	char model;
  } processor_name_string_t;
  static const processor_name_string_t socket_F_table[] = {
	{  1, 0x01, 0x6, "Opteron 22%d HE",			'R' },
	{  1, 0x01, 0xA, "Opteron 22%d",			'R' },
	{  1, 0x01, 0xC, "Opteron 22%d SE",			'R' },
	{  1, 0x04, 0x6, "Opteron 82%d HE",			'R' },
	{  1, 0x04, 0xA, "Opteron 82%d",			'R' },
	{  1, 0x04, 0xC, "Opteron 82%d SE",			'R' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };
  static const processor_name_string_t socket_AM2_table[] = {
	{  0, 0x04, 0x4, "Athlon 64 %d00+",			'T' },
	{  0, 0x04, 0x8, "Athlon 64 %d00+",			'T' },
	{  0, 0x06, 0x4, "Sempron %d00+",			'T' },
	{  0, 0x06, 0x8, "Sempron %d00+",			'T' },
	{  1, 0x01, 0xA, "Opteron 12%d",			'R' },
	{  1, 0x01, 0xC, "Opteron 12%d SE",			'R' },
	{  1, 0x04, 0x2, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x04, 0x6, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x04, 0x8, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x05, 0xC, "Athlon 64 FX-%d",			'Z' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };
  static const processor_name_string_t socket_S1_table[] = {
	{  1, 0x02, 0xC, "Turion 64 X2 TL-%d",		'Y' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };

  const processor_name_string_t *model_names = NULL;
  switch (cpuinfo_get_socket(cip)) {
  case CPUINFO_SOCKET_F:
	model_names = socket_F_table;
	break;
  case CPUINFO_SOCKET_AM2:
	model_names = socket_AM2_table;
	break;
  case CPUINFO_SOCKET_S1:
	model_names = socket_S1_table;
	break;
  }
  if (model_names == NULL)
	return NULL;

  int i;
  for (i = 0; model_names[i].name != NULL; i++) {
	const processor_name_string_t *mp = &model_names[i];
	if ((mp->cmp == -1 || mp->cmp == CmpCap)
		&& mp->index == BrandTableIndex
		&& mp->pwr_lmt == PwrLmt) {
	  int model_number = mp->model;
	  switch (model_number) {
	  case 'R': model_number = -1 + NN; break;
	  case 'P': model_number = 26 + NN; break;
	  case 'T': model_number = 15 + CmpCap * 10 + NN; break;
	  case 'Z': model_number = 57 + NN; break;
	  case 'Y': model_number = 29 + NN; break;
	  }
	  char *model = malloc(64);
	  if (model) {
		if (model_number)
		  sprintf(model, mp->name, model_number);
		else
		  sprintf(model, mp->name);
	  }
	  return model;
	}
  }

  return NULL;
}

static char *get_model_amd(struct cpuinfo *cip)
{
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);
  if (cpuid_level < 1)
	return NULL;

  uint32_t eax, ebx;
  cpuid(1, &eax, &ebx, NULL, NULL);
  uint32_t eightbit_brand_id = ebx & 0xff;

  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) != 0x80000000)
	return NULL;
  if (cpuid_level < 0x80000001)
	return NULL;

  if ((eax & 0xffffff00) == 0x00040f00)
	return get_model_amd_npt(cip);

  uint32_t ecx, edx;
  cpuid(0x80000001, NULL, &ebx, &ecx, &edx);
  uint32_t brand_id = ebx & 0xffff;

  int BrandTableIndex, NN;
  if (eightbit_brand_id != 0) {
	BrandTableIndex = (eightbit_brand_id >> 3) & 0x1c;	// {0b,8BitBrandId[7:5],00b}
	NN = eightbit_brand_id & 0x1f;						// {0b,8BitBrandId[4:0]}
  }
  else if (brand_id == 0) {
	BrandTableIndex = 0;
	NN = 0;
  }
  else {
	BrandTableIndex = (brand_id >> 6) & 0x3f;			// BrandId[11:6]
	NN = brand_id & 0x3f;								// BrandId[5:0]
  }

  static const struct {
	const char *name;
	char model;
  }
  BrandTable[64] = {
	[0x00] = { "Engineering Sample",	    },
	[0x04] = { "Athlon 64 %d00+",		'X' },
	[0x05] = { "Athlon 64 X2 %d00+",	'X' },
	[0x08] = { "Athlon 64 %d00+",		'X' },
	[0x09] = { "Athlon 64 %d00+",		'X' },
	[0x0A] = { "Turion 64 ML-%d",		'X' },
	[0x0B] = { "Turion 64 MT-%d",		'X' },
	[0x0C] = { "Opteron 1%d",			'Y' },
	[0x0D] = { "Opteron 1%d",			'Y' },
	[0x0E] = { "Opteron 1%d HE",		'Y' },
	[0x0F] = { "Opteron 1%d EE",		'Y' },
	[0x10] = { "Opteron 2%d",			'Y' },
	[0x11] = { "Opteron 2%d",			'Y' },
	[0x12] = { "Opteron 2%d HE",		'Y' },
	[0x13] = { "Opteron 2%d EE",		'Y' },
	[0x14] = { "Opteron 8%d",			'Y' },
	[0x15] = { "Opteron 8%d",			'Y' },
	[0x16] = { "Opteron 8%d HE",		'Y' },
	[0x17] = { "Opteron 8%d EE",		'Y' },
	[0x18] = { "Athlon 64 %d00+",		'E' },
	[0x1D] = { "Athlon XP-M %d00+",		'X' },
	[0x1E] = { "Athlon XP-M %d00+",		'X' },
	[0x20] = { "Athlon XP %d00+",		'X' },
	[0x21] = { "Sempron %d00+",			'T' },
	[0x22] = { "Sempron %d00+",			'T' },
	[0x23] = { "Sempron %d00+",			'T' },
	[0x24] = { "Athlon 64 FX-%d",		'Z' },
	[0x26] = { "Sempron %d00+",			'T' },
	[0x29] = { "Opteron 1%d SE",		'R' },
	[0x2A] = { "Opteron 2%d SE",		'R' },
	[0x2B] = { "Opteron 8%d SE",		'R' },
	[0x2C] = { "Opteron 1%d",			'R' },
	[0x2D] = { "Opteron 1%d",			'R' },
	[0x2E] = { "Opteron 1%d HE",		'R' },
	[0x2F] = { "Opteron 1%d EE",		'R' },
	[0x30] = { "Opteron 2%d",			'R' },
	[0x31] = { "Opteron 2%d",			'R' },
	[0x32] = { "Opteron 2%d HE",		'R' },
	[0x33] = { "Opteron 2%d EE",		'R' },
	[0x34] = { "Opteron 8%d",			'R' },
	[0x35] = { "Opteron 8%d",			'R' },
	[0x36] = { "Opteron 8%d HE",		'R' },
	[0x37] = { "Opteron 8%d EE",		'R' },
	[0x38] = { "Opteron 1%d",			'R' },
	[0x39] = { "Opteron 2%d",			'R' },
	[0x3A] = { "Opteron 8%d",			'R' }
  };

  int model_number = BrandTable[BrandTableIndex].model;
  switch (model_number) {
  case 'X': model_number = 22 + NN; break;
  case 'Y': model_number = 38 + (2 * NN); break;
  case 'Z': model_number = 24 + NN; break;
  case 'T': model_number = 24 + NN; break;
  case 'R': model_number = 45 + (5 * NN); break;
  case 'E': model_number = 9 + NN; break;
  }

  const char *name = BrandTable[BrandTableIndex].name;
  if (name == NULL)
	return NULL;

  char *model = malloc(64);
  if (model) {
	if (model_number)
	  sprintf(model, name, model_number);
	else
	  sprintf(model, name);
  }

  return model;
}

// Get Intel processor name
static char *get_model_intel(struct cpuinfo *cip)
{
  return NULL;
}

// Sanitize BrandID string
// FIXME: better go through the troubles of decoding tables to have clean names
static const char *goto_next_block(const char *cp)
{
  while (*cp && !isblank(*cp) && *cp != '(')
	++cp;
  return cp;
}

static const char *skip_blanks(const char *cp)
{
  while (*cp && isblank(*cp))
	++cp;
  return cp;
}

static const char *skip_tokens(const char *cp)
{
  static const char *skip_list[] = {
	"AMD", "Intel",				// processor vendors
	"(TM)", "(R)", "(tm)",		// copyright marks
	"CPU", "Processor", "@",	// superfluous tags
	"Dual-Core", "Genuine",
	NULL
  };
  int i;
  for (i = 0; skip_list[i] != NULL; i++) {
	int len = strlen(skip_list[i]);
	if (strncmp(cp, skip_list[i], len) == 0)
	  return cp + len;
  }
  return cp;
}

static int freq_string(const char *cp, const char *ep)
{
  const char *bp = cp;
  while (cp < ep && (*cp == '.' || isdigit(*cp)))
	++cp;
  return cp != bp && cp + 2 < ep
	&& (cp[0] == 'M' || cp[0] == 'G') && cp[1] == 'H' && cp[2] == 'z';
}

static char *sanitize_brand_id_string(const char *str)
{
  char *model = malloc(64);
  if (model == NULL)
	return NULL;
  const char *cp;
  char *mp = model;
  cp = skip_tokens(skip_tokens(skip_blanks(str))); // skip Vendor(TM)
  do {
	const char *op = cp;
	const char *ep = cp;
	while ((ep = skip_tokens(skip_blanks(ep))) != op)
	  op = ep;
	cp = skip_blanks(ep);
	ep = goto_next_block(cp);
	if (!freq_string(cp, ep)) {
	  if (mp != model)
		*mp++ = ' ';
	  strncpy(mp, cp, ep - cp);
	  mp += ep - cp;
	}
	cp = ep;
  } while (*cp != 0);
  *mp = '\0';
  if (mp == model) {
	free(model);
	model = NULL;
  }
  return model;
}

// Get processor name
char *cpuinfo_arch_get_model(struct cpuinfo *cip)
{
  char *model = NULL;

  switch (cpuinfo_get_vendor(cip)) {
  case CPUINFO_VENDOR_AMD:
	model = get_model_amd(cip);
	break;
  case CPUINFO_VENDOR_INTEL:
	model = get_model_intel(cip);
	break;
  }

  if (model == NULL) {
	uint32_t cpuid_level;
	cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
	if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000004) {
	  D(bug("* cpuinfo_get_model: cpuid(0x80000002)\n"));
	  union { uint32_t r[13]; char str[52]; } m = { { 0, } };
	  cpuid(0x80000002, &m.r[0], &m.r[1], &m.r[2], &m.r[3]);
	  cpuid(0x80000003, &m.r[4], &m.r[5], &m.r[6], &m.r[7]);
	  cpuid(0x80000004, &m.r[8], &m.r[9], &m.r[10], &m.r[11]);
	  model = sanitize_brand_id_string(m.str);
	}
  }

  return model;
}

// Get processor ticks
static inline uint64_t get_ticks(void)
{
  uint32_t low, high;
  __asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
  return (((uint64_t)high) << 32) | low;
}

// Get current value of microsecond timer
static inline uint64_t get_ticks_usec(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t)tv.tv_sec * 1000000) + tv.tv_usec;
}

// Get processor frequency in MHz (x86info)
int cpuinfo_arch_get_frequency(struct cpuinfo *cip)
{
  uint64_t start, stop;
  uint64_t ticks_start, ticks_stop;

  // Make sure TSC is available
  uint32_t edx;
  cpuid(1, NULL, NULL, NULL, &edx);
  if ((edx & (1 << 4)) == 0)
	return 0;

  start = get_ticks_usec();
  ticks_start = get_ticks();
  while ((get_ticks_usec() - start) < 50000) {
	static unsigned long next = 1;
	next = next * 1103515245 + 12345;
  }
  ticks_stop = get_ticks();
  stop = get_ticks_usec();

  uint64_t freq = (ticks_stop - ticks_start) / (stop - start);
  return ((freq % 10) >= 5) ? (((freq / 10) * 10) + 10) : ((freq / 10) * 10);
}

// Get processor socket ID
static int cpuinfo_get_socket_amd(void)
{
  int socket = -1;

  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);
  if ((eax & 0xfff0ff00) == 0x00000f00) {	// AMD K8
	// Factored from AMD Revision Guide, rev 3.59
	switch ((eax >> 4) & 0xf) {
	case 0x4: case 0x8: case 0xc:
	  socket = CPUINFO_SOCKET_754;
	  break;
	case 0x3: case 0x7: case 0xb: case 0xf:
	  socket = CPUINFO_SOCKET_939;
	  break;
	case 0x1: case 0x5:
	  socket = CPUINFO_SOCKET_940;
	  break;
	default:
	  D(bug("* K8 cpuid(1) => %08x\n", eax));
	  break;
	}
	switch ((eax >> 16) & 0xf) {
	case 0x4:								// AMD NPT Family 0Fh (Orleans/Manila)
	  cpuid(0x80000001, &eax, NULL, NULL, NULL);
	  switch ((eax >> 4) & 3) {
	  case 0:
		socket = CPUINFO_SOCKET_S1;
		break;
	  case 1:
		socket = CPUINFO_SOCKET_F;
		break;
	  case 3:
		socket = CPUINFO_SOCKET_AM2;
		break;
	  }
	  break;
	}
  }

  return socket;
}

int cpuinfo_arch_get_socket(struct cpuinfo *cip)
{
  int socket = -1;

  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_AMD)
	socket = cpuinfo_get_socket_amd();

  if (socket < 0)
	socket = cpuinfo_dmi_get_socket(cip);

  return socket;
}

// Get number of cores per CPU package
int cpuinfo_arch_get_cores(struct cpuinfo *cip)
{
  uint32_t eax, ecx;

  /* Intel Dual Core characterisation */
  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_INTEL) {
	cpuid(0, &eax, NULL, NULL, NULL);
	if (eax >= 4) {
	  ecx = 0;
	  cpuid(4, &eax, NULL, &ecx, NULL);
	  return 1 + ((eax >> 26) & 0x3f);
	}
  }

  /* AMD Dual Core characterisation */
  else if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_AMD) {
	cpuid(0x80000000, &eax, NULL, NULL, NULL);
	if (eax >= 0x80000008) {
	  cpuid(0x80000008, NULL, NULL, &ecx, NULL);
	  return 1 + (ecx & 0xff);
	}
  }

  return 1;
}

// Get number of threads per CPU core
int cpuinfo_arch_get_threads(struct cpuinfo *cip)
{
  uint32_t eax, ebx, edx;

  switch (cpuinfo_get_vendor(cip)) {
  case CPUINFO_VENDOR_INTEL:
	/* Check for Hyper Threading Technology activated */
	/* See "Intel Processor Identification and the CPUID Instruction" (3.3 Feature Flags) */
	cpuid(0, &eax, NULL, NULL, NULL);
	if (eax >= 1) {
	  cpuid(1, NULL, &ebx, NULL, &edx);
	  if (edx & (1 << 28)) { /* HTT flag */
		int n_cores = cpuinfo_get_cores(cip);
		assert(n_cores > 0);
		return ((ebx >> 16) & 0xff) / n_cores;
	  }
	}
	break;
  }

  return 1;
}

// Get cache information (initialize with iter = 0, returns the
// iteration number or -1 if no more information available)
// Reference: Application Note 485 -- Intel Processor Identification
static const struct {
  uint8_t desc;
  uint8_t level;
  uint8_t type;
  uint16_t size;
}
intel_cache_table[] = {
#define C_(LEVEL, TYPE) LEVEL, CPUINFO_CACHE_TYPE_##TYPE
  { 0x06, C_(1, CODE),		    8 }, // 4-way set assoc, 32 byte line size
  { 0x08, C_(1, CODE),		   16 }, // 4-way set assoc, 32 byte line size
  { 0x0a, C_(1, DATA),		    8 }, // 2 way set assoc, 32 byte line size
  { 0x0c, C_(1, DATA),		   16 }, // 4-way set assoc, 32 byte line size
  { 0x22, C_(3, UNIFIED),	  512 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x23, C_(3, UNIFIED),	 1024 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x25, C_(3, UNIFIED),	 2048 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x29, C_(3, UNIFIED),	 4096 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x2c, C_(1, DATA),		   32 }, // 8-way set assoc, 64 byte line size
  { 0x30, C_(1, CODE),		   32 }, // 8-way set assoc, 64 byte line size
  { 0x39, C_(2, UNIFIED),	  128 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x3a, C_(2, UNIFIED),	  192 }, // 6-way set assoc, sectored cache, 64 byte line size
  { 0x3b, C_(2, UNIFIED),	  128 }, // 2-way set assoc, sectored cache, 64 byte line size
  { 0x3c, C_(2, UNIFIED),	  256 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x3d, C_(2, UNIFIED),	  384 }, // 6-way set assoc, sectored cache, 64 byte line size
  { 0x3e, C_(2, UNIFIED),	  512 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x41, C_(2, UNIFIED),	  128 }, // 4-way set assoc, 32 byte line size
  { 0x42, C_(2, UNIFIED),	  256 }, // 4-way set assoc, 32 byte line size
  { 0x43, C_(2, UNIFIED),	  512 }, // 4-way set assoc, 32 byte line size
  { 0x44, C_(2, UNIFIED),	 1024 }, // 4-way set assoc, 32 byte line size
  { 0x45, C_(2, UNIFIED),	 2048 }, // 4-way set assoc, 32 byte line size
  { 0x46, C_(3, UNIFIED),	 4096 }, // 4-way set assoc, 64 byte line size
  { 0x47, C_(3, UNIFIED),	 8192 }, // 8-way set assoc, 64 byte line size
  { 0x49, C_(3, UNIFIED),	 4096 }, // 16-way set assoc, 64 byte line size
  { 0x4a, C_(3, UNIFIED),	 6144 }, // 12-way set assoc, 64 byte line size
  { 0x4b, C_(3, UNIFIED),	 8192 }, // 16-way set assoc, 64 byte line size
  { 0x4c, C_(3, UNIFIED),	12288 }, // 12-way set assoc, 64 byte line size
  { 0x4d, C_(3, UNIFIED),	16384 }, // 16-way set assoc, 64 byte line size
  { 0x60, C_(1, DATA),		   16 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x66, C_(1, DATA),		    8 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x67, C_(1, DATA),		   16 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x68, C_(1, DATA),		   32 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x70, C_(0, TRACE),		   12 }, // 8-way set assoc
  { 0x71, C_(0, TRACE),		   16 }, // 8-way set assoc
  { 0x72, C_(0, TRACE),		   32 }, // 8-way set assoc
  { 0x73, C_(0, TRACE),		   64 }, // 8-way set assoc
  { 0x78, C_(2, UNIFIED),	 1024 }, // 4-way set assoc, 64 byte line size
  { 0x79, C_(2, UNIFIED),	  128 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7a, C_(2, UNIFIED),	  256 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7b, C_(2, UNIFIED),	  512 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7c, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7d, C_(2, UNIFIED),	 2048 }, // 8-way set assoc, 64 byte line size
  { 0x7f, C_(2, UNIFIED),	  512 }, // 2-way set assoc, 64 byte line size
  { 0x82, C_(2, UNIFIED),	  256 }, // 8-way set assoc, 32 byte line size
  { 0x83, C_(2, UNIFIED),	  512 }, // 8-way set assoc, 32 byte line size
  { 0x84, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, 32 byte line size
  { 0x85, C_(2, UNIFIED),	 2048 }, // 8-way set assoc, 32 byte line size
  { 0x86, C_(2, UNIFIED),	  512 }, // 4-way set assoc, 64 byte line size
  { 0x87, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, 64 byte line size
  { 0x00, C_(0, UNKNOWN),	    0 }
#undef C_
};

cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip)
{
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);

  cpuinfo_list_t caches_list = NULL;
  cpuinfo_cache_descriptor_t cache_desc;

  if (cpuid_level >= 4) {
	// XXX not MP safe cpuid()
	D(bug("* cpuinfo_get_cache: cpuid(4)\n"));
	uint32_t eax, ebx, ecx, edx;
	int count = 0;
	for (;;) {
	  ecx = count;
	  cpuid(4, &eax, &ebx, &ecx, &edx);
	  int cache_type = eax & 0x1f;
	  if (cache_type == 0)
		break;
	  switch (cache_type) {
	  case 1: cache_type = CPUINFO_CACHE_TYPE_DATA; break;
	  case 2: cache_type = CPUINFO_CACHE_TYPE_CODE; break;
	  case 3: cache_type = CPUINFO_CACHE_TYPE_UNIFIED; break;
	  default: cache_type = CPUINFO_CACHE_TYPE_UNKNOWN; break;
	  }
	  cache_desc.type = cache_type;
	  cache_desc.level = (eax >> 5) & 7;
	  uint32_t W = 1 + ((ebx >> 22) & 0x3f);	// ways of associativity
	  uint32_t P = 1 + ((ebx >> 12) & 0x1f);	// physical line partition
	  uint32_t L = 1 + (ebx & 0xfff);			// system coherency line size
	  uint32_t S = 1 + ecx;						// number of sets
	  cache_desc.size = (L * W * P * S) / 1024;
	  cpuinfo_caches_list_insert(&cache_desc);
	  ++count;
	}
	return caches_list;
  }

  if (cpuid_level >= 2) {
	int i, j, k, n;
	uint32_t regs[4];
	uint8_t *dp = (uint8_t *)regs;
	D(bug("* cpuinfo_get_cache: cpuid(2)\n"));
	cpuid(2, &regs[0], NULL, NULL, NULL);
	n = regs[0] & 0xff;						// number of times to iterate
	for (i = 0; i < n; i++) {
	  cpuid(2, &regs[0], &regs[1], &regs[2], &regs[3]);
	  for (j = 0; j < 4; j++) {
		if (regs[j] & 0x80000000)
		  regs[j] = 0;
	  }
	  for (j = 1; j < 16; j++) {
		uint8_t desc = dp[j];
		for (k = 0; intel_cache_table[k].desc != 0; k++) {
		  if (intel_cache_table[k].desc == desc) {
			cache_desc.type = intel_cache_table[k].type;
			cache_desc.level = intel_cache_table[k].level;
			cache_desc.size = intel_cache_table[k].size;
			cpuinfo_caches_list_insert(&cache_desc);
			D(bug("* %02x\n", desc));
			break;
		  }
		}
	  }
	}
	return caches_list;
  }

  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000005) {
	uint32_t ecx, edx;
	D(bug("* cpuinfo_get_cache: cpuid(0x80000005)\n"));
	cpuid(0x80000005, NULL, NULL, &ecx, &edx);
	cache_desc.level = 1;
	cache_desc.type = CPUINFO_CACHE_TYPE_CODE;
	cache_desc.size = (edx >> 24) & 0xff;
	cpuinfo_caches_list_insert(&cache_desc);
	cache_desc.level = 1;
	cache_desc.type = CPUINFO_CACHE_TYPE_DATA;
	cache_desc.size = (ecx >> 24) & 0xff;
	cpuinfo_caches_list_insert(&cache_desc);
	if (cpuid_level >= 0x80000006) {
	  D(bug("* cpuinfo_get_cache: cpuid(0x80000006)\n"));
	  cpuid(0x80000006, NULL, NULL, &ecx, NULL);
	  if (((ecx >> 12) & 0xffff) != 0) {
		cache_desc.level = 2;
		cache_desc.type = CPUINFO_CACHE_TYPE_UNIFIED;
		cache_desc.size = (ecx >> 16) & 0xffff;
		cpuinfo_caches_list_insert(&cache_desc);
	  }
	}
	return caches_list;
  }

  return cpuinfo_dmi_get_caches(cip);
}

static int bsf_clobbers_eflags(void)
{
  int mismatch = 0;
  int g_ZF, g_CF, g_OF, g_SF, value;
  for (g_ZF = 0; g_ZF <= 1; g_ZF++) {
	for (g_CF = 0; g_CF <= 1; g_CF++) {
	  for (g_OF = 0; g_OF <= 1; g_OF++) {
		for (g_SF = 0; g_SF <= 1; g_SF++) {
		  for (value = -1; value <= 1; value++) {
			unsigned long flags = (g_SF << 7) | (g_OF << 11) | (g_ZF << 6) | g_CF;
			unsigned long tmp = value;
			__asm__ __volatile__ ("push %0; popf; bsf %1,%1; pushf; pop %0"
								  : "+r" (flags), "+r" (tmp) : : "cc");
			int OF = (flags >> 11) & 1;
			int SF = (flags >>  7) & 1;
			int ZF = (flags >>  6) & 1;
			int CF = flags & 1;
			tmp = (value == 0);
			if (ZF != tmp || SF != g_SF || OF != g_OF || CF != g_CF)
				mismatch = 1;
		  }
		}
	  }
	}
  }
  return mismatch;
}

// Returns features table
uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature)
{
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	return cip->features;
  case CPUINFO_FEATURE_X86:
	return ((x86_cpuinfo_t *)(cip->opaque))->features;
  }
  return NULL;
}

#define feature_get_bit(NAME) cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_X86_##NAME)
#define feature_set_bit(NAME) cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_X86_##NAME)

// Returns 0 if CPU supports the specified feature
int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature)
{
  if (!cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_X86)) {
	cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_X86);

	uint32_t eax, ecx, edx;
	cpuid(1, NULL, NULL, &ecx, &edx);
	if (edx & (1 << 15))
	  feature_set_bit(CMOV);
	if (edx & (1 << 23))
	  feature_set_bit(MMX);
	if (edx & (1 << 25))
	  feature_set_bit(SSE);
	if (edx & (1 << 26))
	  feature_set_bit(SSE2);
	if (ecx & (1 << 0))
	  feature_set_bit(SSE3);
	if (ecx & (1 << 9))
	  feature_set_bit(SSSE3);
	if (ecx & (1 << 5))
	  feature_set_bit(VMX);

	cpuid(0x80000000, &eax, NULL, NULL, NULL);
	if ((eax & 0xffff0000) == 0x80000000 && eax >= 0x80000001) {
	  cpuid(0x80000001, NULL, NULL, &ecx, &edx);
	  if (ecx & (1 << 2))
		feature_set_bit(SVM);
	  if (ecx & (1 << 0))
		feature_set_bit(LAHF64);
	  if (edx & (1 << 29))
		feature_set_bit(LM);
	}

	if (bsf_clobbers_eflags())
	  feature_set_bit(BSFCC);

	if (feature_get_bit(LM))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_64BIT);

	if (feature_get_bit(MMX) ||
		feature_get_bit(SSE) ||
		feature_get_bit(SSE2) ||
		feature_get_bit(SSE3) ||
		feature_get_bit(SSSE3) ||
		feature_get_bit(SSE4))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_SIMD);
  }

  return cpuinfo_feature_get_bit(cip, feature);
}

#endif