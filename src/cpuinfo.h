/*
 *  cpuinfo.h - Public interface
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

#ifndef CPUINFO_H
#define CPUINFO_H

#ifdef __cplusplus
extern "C" {
#endif

// cpuinfo_t is a private data structure
typedef struct cpuinfo cpuinfo_t;

// Returns a new cpuinfo descriptor
extern cpuinfo_t *cpuinfo_new(void);

// Release the cpuinfo descriptor and all allocated data
extern void cpuinfo_destroy(cpuinfo_t *cip);

// Dump all useful information for debugging
extern int cpuinfo_dump(cpuinfo_t *cip, FILE *out);

/* ========================================================================= */
/* == General Processor Information                                       == */
/* ========================================================================= */

#define CPUINFO_CLASS(N) (((unsigned int)(N)) << 8)

// Processor vendor
typedef enum {
  CPUINFO_VENDOR_UNKNOWN,
  CPUINFO_VENDOR_AMD,
  CPUINFO_VENDOR_CENTAUR,
  CPUINFO_VENDOR_CYRIX,
  CPUINFO_VENDOR_IBM,
  CPUINFO_VENDOR_INTEL,
  CPUINFO_VENDOR_MOTOROLA,
  CPUINFO_VENDOR_MIPS,
  CPUINFO_VENDOR_NEXTGEN,
  CPUINFO_VENDOR_NSC,
  CPUINFO_VENDOR_PMC,
  CPUINFO_VENDOR_RISE,
  CPUINFO_VENDOR_SIS,
  CPUINFO_VENDOR_TRANSMETA,
  CPUINFO_VENDOR_UMC,
  CPUINFO_VENDOR_PASEMI
} cpuinfo_vendor_t;

// Get processor vendor ID 
extern int cpuinfo_get_vendor(cpuinfo_t *cip);

// Get processor name
extern const char *cpuinfo_get_model(cpuinfo_t *cip);

// Processor socket
typedef enum {
  CPUINFO_SOCKET_UNKNOWN,

  CPUINFO_SOCKET_478 = CPUINFO_CLASS('I'),
  CPUINFO_SOCKET_479,
  CPUINFO_SOCKET_604,
  CPUINFO_SOCKET_771,
  CPUINFO_SOCKET_775,

  CPUINFO_SOCKET_754 = CPUINFO_CLASS('A'),
  CPUINFO_SOCKET_939,
  CPUINFO_SOCKET_940,
  CPUINFO_SOCKET_AM2,
  CPUINFO_SOCKET_F,
  CPUINFO_SOCKET_S1
} cpuinfo_socket_t;

// Get processor frequency in MHz
extern int cpuinfo_get_frequency(cpuinfo_t *cip);

// Get processor socket ID
extern int cpuinfo_get_socket(cpuinfo_t *cip);

// Get number of cores per CPU package
extern int cpuinfo_get_cores(cpuinfo_t *cip);

// Get number of threads per CPU core
extern int cpuinfo_get_threads(cpuinfo_t *cip);

/* ========================================================================= */
/* == Processor Caches Information                                        == */
/* ========================================================================= */

typedef enum {
  CPUINFO_CACHE_TYPE_UNKNOWN,
  CPUINFO_CACHE_TYPE_DATA,
  CPUINFO_CACHE_TYPE_CODE,
  CPUINFO_CACHE_TYPE_UNIFIED,
  CPUINFO_CACHE_TYPE_TRACE
} cpuinfo_cache_type_t;

typedef struct {
  int type;		// cache type (above)
  int level;	// cache level
  int size;		// cache size in KB
} cpuinfo_cache_descriptor_t;

typedef struct {
  int count;	// number of cache descriptors
  const cpuinfo_cache_descriptor_t *descriptors;
} cpuinfo_cache_t;

// Get cache information (returns read-only descriptors)
extern const cpuinfo_cache_t *cpuinfo_get_caches(cpuinfo_t *cip);

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

typedef enum {
  CPUINFO_FEATURE_ARCH	= 0xff00,
  CPUINFO_FEATURE_MASK	= 0x00ff,

  CPUINFO_FEATURE_COMMON = 0,
  CPUINFO_FEATURE_64BIT,
  CPUINFO_FEATURE_SIMD,
  CPUINFO_FEATURE_POPCOUNT,
  CPUINFO_FEATURE_COMMON_MAX,

  CPUINFO_FEATURE_X86	= CPUINFO_CLASS('X'),
  CPUINFO_FEATURE_X86_CMOV,
  CPUINFO_FEATURE_X86_MMX,
  CPUINFO_FEATURE_X86_MMX_PLUS,
  CPUINFO_FEATURE_X86_3DNOW,
  CPUINFO_FEATURE_X86_3DNOW_PLUS,
  CPUINFO_FEATURE_X86_SSE,
  CPUINFO_FEATURE_X86_SSE2,
  CPUINFO_FEATURE_X86_SSE3,
  CPUINFO_FEATURE_X86_SSSE3,
  CPUINFO_FEATURE_X86_SSE4_1,
  CPUINFO_FEATURE_X86_SSE4_2,
  CPUINFO_FEATURE_X86_SSE4A,
  CPUINFO_FEATURE_X86_SSE5,
  CPUINFO_FEATURE_X86_MSSE,
  CPUINFO_FEATURE_X86_VMX,
  CPUINFO_FEATURE_X86_SVM,
  CPUINFO_FEATURE_X86_LM,
  CPUINFO_FEATURE_X86_LAHF64,
  CPUINFO_FEATURE_X86_POPCNT,
  CPUINFO_FEATURE_X86_ABM,
  CPUINFO_FEATURE_X86_BSFCC,
  CPUINFO_FEATURE_X86_TM,
  CPUINFO_FEATURE_X86_TM2,
  CPUINFO_FEATURE_X86_EIST,
  CPUINFO_FEATURE_X86_NX,
  CPUINFO_FEATURE_X86_MAX,

  CPUINFO_FEATURE_IA64	= CPUINFO_CLASS('I'),
  CPUINFO_FEATURE_IA64_LB,
  CPUINFO_FEATURE_IA64_SD,
  CPUINFO_FEATURE_IA64_AO,
  CPUINFO_FEATURE_IA64_MAX,

  CPUINFO_FEATURE_PPC	= CPUINFO_CLASS('P'),
  CPUINFO_FEATURE_PPC_VMX,
  CPUINFO_FEATURE_PPC_FSQRT,
  CPUINFO_FEATURE_PPC_FSEL,
  CPUINFO_FEATURE_PPC_MFCRF,
  CPUINFO_FEATURE_PPC_POPCNTB,
  CPUINFO_FEATURE_PPC_FRIZ,
  CPUINFO_FEATURE_PPC_MFPGPR,
  CPUINFO_FEATURE_PPC_MAX,
  CPUINFO_FEATURE_PPC_GPOPT		= CPUINFO_FEATURE_PPC_FSQRT,
  CPUINFO_FEATURE_PPC_GFXOPT	= CPUINFO_FEATURE_PPC_FSEL,
  CPUINFO_FEATURE_PPC_FPRND		= CPUINFO_FEATURE_PPC_FRIZ,

  CPUINFO_FEATURE_MIPS	= CPUINFO_CLASS('M'),
  CPUINFO_FEATURE_MIPS_MAX
} cpuinfo_feature_t;

// Returns 1 if CPU supports the specified feature
extern int cpuinfo_has_feature(cpuinfo_t *cip, int feature);

// Utility functions to convert IDs
extern const char *cpuinfo_string_of_vendor(int vendor);
extern const char *cpuinfo_string_of_socket(int socket);
extern const char *cpuinfo_string_of_cache_type(int cache_type);
extern const char *cpuinfo_string_of_feature(int feature);
extern const char *cpuinfo_string_of_feature_detail(int feature);

#ifdef __cplusplus
}
#endif

#endif /* CPUINFO_H */
