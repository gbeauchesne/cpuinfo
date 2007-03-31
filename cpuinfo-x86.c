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
  __asm__ __volatile__ ("push	%%ebx\n\t"
						"cpuid	\n\t"
						"mov	%%ebx,%0\n\t"
						"pop	%%ebx"
						: "=r" (b), "=a" (a), "=c" (c), "=d" (d)
						: "0" (op), "2" (c));
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

// Get processor vendor ID 
int cpuinfo_get_vendor(void)
{
  static int vendor = -1;

  if (vendor < 0) {
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
  }

  return vendor;
}

// Get AMD processor name
static const char *get_model_amd(void)
{
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);
  if (cpuid_level < 1)
	return NULL;

  uint32_t ebx;
  cpuid(1, NULL, &ebx, NULL, NULL);
  uint32_t brand_id = ebx & 0xff;

  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) != 0x80000000)
	return NULL;
  if (cpuid_level < 0x80000001)
	  return NULL;

  uint32_t ecx, edx;
  cpuid(0x80000001, NULL, &ebx, &ecx, &edx);
  if (brand_id != 0 || ebx == 0) // Ensure 8BitBrandId == 0, BrandId == non-zero
	return NULL;
  D(bug("* cpuinfo_get_model: AMD BrandId\n"));

  int BrandTableIndex = (ebx >> 6) & 0x3f;
  int NN = ebx & 0x3f;
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

  static char model[64];
  if (model)
	sprintf(model, name, model_number);
  else
	sprintf(model, name);

  return model;
}

// Get Intel processor name
static const char *get_model_intel(void)
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
	"CPU", "Processor",			// superfluous tags
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

static const char *sanitize_brand_id_string(const char *str)
{
  static char model[49];
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
  return model;
}

// Get processor name
const char *cpuinfo_get_model(void)
{
  static const char *model = NULL;

  if (model == NULL) {
	int vendor = cpuinfo_get_vendor();
	switch (vendor) {
	case CPUINFO_VENDOR_AMD:
	  model = get_model_amd();
	  break;
	case CPUINFO_VENDOR_INTEL:
	  model = get_model_intel();
	  break;
	}
  }

  if (model == NULL) {
	uint32_t cpuid_level;
	cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
	if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000004) {
	  D(bug("* cpuinfo_get_model: cpuid(0x80000002)\n"));
	  union { uint32_t r[13]; char str[52]; } m = { 0, };
	  cpuid(0x80000002, &m.r[0], &m.r[1], &m.r[2], &m.r[3]);
	  cpuid(0x80000003, &m.r[4], &m.r[5], &m.r[6], &m.r[7]);
	  cpuid(0x80000004, &m.r[8], &m.r[9], &m.r[10], &m.r[11]);
	  model = sanitize_brand_id_string(m.str);
	}
  }

  if (model == NULL)
	model = "<unknown>";

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
int cpuinfo_get_frequency(void)
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
int cpuinfo_get_socket(void)
{
  static int socket = -1;

  if (socket < 0) {
	socket = CPUINFO_SOCKET_UNKNOWN;

	if (cpuinfo_get_vendor() == CPUINFO_VENDOR_AMD) {
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
	  }
	}

	if (socket == CPUINFO_SOCKET_UNKNOWN)
	  socket = cpuinfo_dmi_get_socket();
  }

  return socket;
}

// Get number of cores per CPU package
int cpuinfo_get_cores(void)
{
  static int n_cores = -1;

  if (n_cores < 0) {
	n_cores = 1;
	uint32_t eax, ebx, ecx, edx;

	/* Intel Dual Core characterisation */
	if (cpuinfo_get_vendor() == CPUINFO_VENDOR_INTEL) {
	  cpuid(0, &eax, NULL, NULL, NULL);
	  if (eax >= 4) {
		ecx = 0;
		cpuid(4, &eax, NULL, &ecx, NULL);
		n_cores = 1 + ((eax >> 26) & 0x3f);
	  }
	}

	/* AMD Dual Core characterisation */
	else if (cpuinfo_get_vendor() == CPUINFO_VENDOR_AMD) {
	  cpuid(0x80000000, &eax, NULL, NULL, NULL);
	  if (eax >= 0x80000008) {
		cpuid(0x80000008, NULL, NULL, &ecx, NULL);
		n_cores = 1 + (ecx & 0xff);
	  }
    }
  }

  return n_cores;
}

// Get number of threads per CPU core
int cpuinfo_get_threads(void)
{
  uint32_t eax, ebx, ecx, edx;

  if (cpuinfo_get_vendor() != CPUINFO_VENDOR_INTEL)
	return 1;

  /* Check for Hyper Threading Technology activated */
  /* See "Intel Processor Identification and the CPUID Instruction" (3.3 Feature Flags) */
  cpuid(0, &eax, NULL, NULL, NULL);
  if (eax >= 1) {
	cpuid(1, NULL, &ebx, NULL, &edx);
	if (edx & (1 << 28)) { /* HTT flag */
	  int n_cores = cpuinfo_get_cores();
	  assert(n_cores > 0);
	  int n_threads = ((ebx >> 16) & 0xff) / n_cores;
	  if (n_threads > 1)
		return n_threads;
	}
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

int cpuinfo_get_cache(int iter, cpuinfo_cache_t *cip)
{
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);

  if (cpuid_level >= 4) {
	// XXX not MP safe cpuid()
	D(bug("* cpuinfo_get_cache: cpuid(4)\n"));
	uint32_t eax, ebx, ecx, edx;
	ecx = iter;
	cpuid(4, &eax, &ebx, &ecx, &edx);
	int cache_type = eax & 0x1f;
	if (cache_type == 0)
	  return -1;
	switch (cache_type) {
	case 1: cache_type = CPUINFO_CACHE_TYPE_DATA; break;
	case 2: cache_type = CPUINFO_CACHE_TYPE_CODE; break;
	case 3: cache_type = CPUINFO_CACHE_TYPE_UNIFIED; break;
	default: cache_type = CPUINFO_CACHE_TYPE_UNKNOWN; break;
	}
	if (cip) {
	  cip->type = cache_type;
	  cip->level = (eax >> 5) & 7;
      uint32_t W = 1 + ((ebx >> 22) & 0x3f);	// ways of associativity
      uint32_t P = 1 + ((ebx >> 12) & 0x1f);	// physical line partition
      uint32_t L = 1 + (ebx & 0xfff);			// system coherency line size
      uint32_t S = 1 + ecx;						// number of sets
	  cip->size = (L * W * P * S) / 1024;
	}
	return iter + 1;
  }

  if (cpuid_level >= 2) {
	static cpuinfo_cache_t ci[4];				// maximum number of caches
	static int ci_count = -1;
	if (ci_count < 0) {
	  int i, j, k, n;
	  uint32_t regs[4];
	  uint8_t *dp = (uint8_t *)regs;
	  D(bug("* cpuinfo_get_cache: cpuid(2)\n"));
	  cpuid(2, &regs[0], NULL, NULL, NULL);
	  n = regs[0] & 0xff;						// number of times to iterate
	  ci_count = 0;
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
			  ci[ci_count].type = intel_cache_table[k].type;
			  ci[ci_count].level = intel_cache_table[k].level;
			  ci[ci_count].size = intel_cache_table[k].size;
			  D(bug("* %02x\n", desc));
			  ci_count++;
			  break;
			}
		  }
		}
	  }
	}
	if (iter >= 0 && iter < ci_count) {
	  if (cip)
		*cip = ci[iter];
	  return iter + 1;
	}
  }

  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000005) {
	static cpuinfo_cache_t ci[3];
	static int ci_count = -1;
	if (ci_count < 0) {
	  uint32_t ecx, edx;
	  D(bug("* cpuinfo_get_cache: cpuid(0x80000005)\n"));
	  cpuid(0x80000005, NULL, NULL, &ecx, &edx);
	  ci[0].level = 1;
	  ci[0].type = CPUINFO_CACHE_TYPE_CODE;
	  ci[0].size = (edx >> 24) & 0xff;
	  ci[1].level = 1;
	  ci[1].type = CPUINFO_CACHE_TYPE_DATA;
	  ci[1].size = (ecx >> 24) & 0xff;
	  ci_count = 2;
	  if (cpuid_level >= 0x80000006) {
		D(bug("* cpuinfo_get_cache: cpuid(0x80000006)\n"));
		cpuid(0x80000006, NULL, NULL, &ecx, NULL);
		if (((ecx >> 12) & 0xffff) != 0) {
		  ci[2].level = 2;
		  ci[2].type = CPUINFO_CACHE_TYPE_UNIFIED;
		  ci[2].size = (ecx >> 16) & 0xffff;
		  ci_count = 3;
		}
	  }
	}
	if (iter >= 0 && iter < ci_count) {
	  if (cip)
		*cip = ci[iter];
	  return iter + 1;
	}
  }

  return cpuinfo_dmi_get_cache(iter, cip);
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

#define feature_get_bit(NAME) cpuinfo_feature_get_bit(CPUINFO_FEATURE_X86_##NAME)
#define feature_set_bit(NAME) cpuinfo_feature_set_bit(CPUINFO_FEATURE_X86_##NAME)

// Returns 0 if CPU supports the specified feature
int cpuinfo_has_feature(int feature)
{
  static int probed_features = 0;

  if (!probed_features) {
	cpuinfo_feature_set_bit(CPUINFO_FEATURE_X86);

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
	  cpuinfo_feature_set_bit(CPUINFO_FEATURE_64BIT);

	if (feature_get_bit(MMX) ||
		feature_get_bit(SSE) ||
		feature_get_bit(SSE2) ||
		feature_get_bit(SSE3) ||
		feature_get_bit(SSSE3) ||
		feature_get_bit(SSE4))
	  cpuinfo_feature_set_bit(CPUINFO_FEATURE_SIMD);

	probed_features = 1;
  }

  return cpuinfo_feature_get_bit(feature);
}

#endif
