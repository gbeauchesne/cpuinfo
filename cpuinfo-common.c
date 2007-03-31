/*
 *  cpuinfo-common.c - Common utility functions
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
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#define DEBUG 0
#include "debug.h"


/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

#define CPUINFO_SZ_(NAME) (1 + ((CPUINFO_FEATURE_##NAME##_MAX - CPUINFO_FEATURE_##NAME) / 32))
static uint32_t common_features[CPUINFO_SZ_(COMMON)];
static uint32_t x86_features[CPUINFO_SZ_(X86)];
static uint32_t ppc_features[CPUINFO_SZ_(PPC)];
#undef CPUINFO_SZ_

static uint32_t *cpuinfo_feature_table(int feature)
{
  uint32_t *ftp = NULL;
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	ftp = common_features;
	break;
  case CPUINFO_FEATURE_X86:
	ftp = x86_features;
	break;
  case CPUINFO_FEATURE_PPC:
	ftp = ppc_features;
	break;
  }
  return ftp;
}

int cpuinfo_feature_get_bit(int feature)
{
  uint32_t *ftp = cpuinfo_feature_table(feature);
  if (ftp) {
	feature &= CPUINFO_FEATURE_MASK;
	return ftp[feature / 32] & (1 << (feature % 32));
  }
  return 0;
}


/* ========================================================================= */
/* == Stringification of CPU Information bits                             == */
/* ========================================================================= */

void cpuinfo_feature_set_bit(int feature)
{
  uint32_t *ftp = cpuinfo_feature_table(feature);
  if (ftp) {
	feature &= CPUINFO_FEATURE_MASK;
	ftp[feature / 32] |= 1 << (feature % 32);
  }
}

const char *cpuinfo_string_of_vendor(int vendor)
{
  const char *str = "<unknown>";
  switch (vendor) {
  case CPUINFO_VENDOR_AMD:			str = "AMD";		break;
  case CPUINFO_VENDOR_IBM:			str = "IBM";		break;
  case CPUINFO_VENDOR_INTEL:		str = "Intel";		break;
  case CPUINFO_VENDOR_MOTOROLA:		str = "Motorola";	break;
  case CPUINFO_VENDOR_TRANSMETA:	str = "Transmeta";	break;
  }
  return str;
}

const char *cpuinfo_string_of_socket(int socket)
{
  const char *str = "Socket <unknown>";
  switch (socket) {
  case CPUINFO_SOCKET_478:		str = "Socket 478";		break;
  case CPUINFO_SOCKET_479:		str = "Socket 479";		break;
  case CPUINFO_SOCKET_604:		str = "Socket mPGA604";	break;
  case CPUINFO_SOCKET_771:		str = "Socket LGA771";	break;
  case CPUINFO_SOCKET_775:		str = "Socket LGA775";	break;
  case CPUINFO_SOCKET_754:		str = "Socket 754";		break;
  case CPUINFO_SOCKET_939:		str = "Socket 939";		break;
  case CPUINFO_SOCKET_940:		str = "Socket 940";		break;
  case CPUINFO_SOCKET_AM2:		str = "Socket AM2";		break;
  }
  return str;
}

const char *cpuinfo_string_of_cache_type(int cache_type)
{
  const char *str = "<unknown>";
  switch (cache_type) {
  case CPUINFO_CACHE_TYPE_DATA:		str = "data";		break;
  case CPUINFO_CACHE_TYPE_CODE:		str = "code";		break;
  case CPUINFO_CACHE_TYPE_UNIFIED:	str = "unified";	break;
  case CPUINFO_CACHE_TYPE_TRACE:	str = "trace";		break;
  }
  return str;
}

typedef struct {
  const char *name;
  const char *detail;
} cpuinfo_feature_string_t;

static const cpuinfo_feature_string_t common_feature_strings[] = {
#define CPUINFO_(NAME) (CPUINFO_FEATURE_##NAME & CPUINFO_FEATURE_MASK)
  [CPUINFO_(64BIT)] = { "64bit", "64-bit capable" },
  [CPUINFO_(SIMD)] = { "simd", "SIMD capable" },
#undef CPUINFO_
};

static const cpuinfo_feature_string_t x86_feature_strings[] = {
  { "[x86]", "-- x86-specific features --" },
#define CPUINFO_(NAME) (CPUINFO_FEATURE_X86_##NAME & CPUINFO_FEATURE_MASK)
  [CPUINFO_(CMOV)] = { "cmov", "Conditional Moves" },
  [CPUINFO_(MMX)] = { "mmx", "MMX Technology" },
  [CPUINFO_(SSE)] = { "sse", "SSE Technology" },
  [CPUINFO_(SSE2)] = { "sse2", "SSE2 Technology" },
  [CPUINFO_(SSE3)] = { "pni", "SSE3 Technology (Prescott New Instructions)" },
  [CPUINFO_(SSSE3)] = { "mni", "SSSE3 Technology (Merom New Instructions)" },
  [CPUINFO_(SSE4)] = { "nni", "SSE4 Technology (Nehalem New Instructions)" },
  [CPUINFO_(VMX)] = { "vmx", "Intel Virtualisation Technology (VT)" },
  [CPUINFO_(SVM)] = { "svm", "AMD-v Technology (Pacifica)" },
  [CPUINFO_(LM)]= { "lm", "Long Mode (64-bit capable)" },
  [CPUINFO_(LAHF64)] = { "lahf_lm", "LAHF/SAHF Supported in 64-bit mode" },
  [CPUINFO_(BSFCC)] = { "bsf_cc", "BSF instruction clobbers condition codes" },
#undef CPUINFO_
};

static const cpuinfo_feature_string_t ppc_feature_strings[] = {
  { "[ppc]", "-- ppc-specific features --" },
#define CPUINFO_(NAME) (CPUINFO_FEATURE_PPC_##NAME & CPUINFO_FEATURE_MASK)
  [CPUINFO_(ALTIVEC)] = { "altivec", "Altivec" },
#undef CPUINFO_
};

static const cpuinfo_feature_string_t *cpuinfo_feature_string_ptr(int feature)
{
  const cpuinfo_feature_string_t *fsp = NULL;
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	fsp = common_feature_strings;
	break;
  case CPUINFO_FEATURE_X86:
	fsp = x86_feature_strings;
	break;
  case CPUINFO_FEATURE_PPC:
	fsp = ppc_feature_strings;
	break;
  }
  return fsp ? &fsp[feature & CPUINFO_FEATURE_MASK] : NULL;
}

const char *cpuinfo_string_of_feature(int feature)
{
  const cpuinfo_feature_string_t *fsp = cpuinfo_feature_string_ptr(feature);
  return fsp && fsp->name ? fsp->name : "<unknown>";
}

const char *cpuinfo_string_of_feature_detail(int feature)
{
  const cpuinfo_feature_string_t *fsp = cpuinfo_feature_string_ptr(feature);
  return fsp->detail ? fsp->detail : "<unknown>";
}
