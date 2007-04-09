/*
 *  cpuinfo.h - Public interface
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

#ifndef CPUINFO_H
#define CPUINFO_H

// cpuinfo_t is a private data structure
struct cpuinfo;

// Returns a new cpuinfo descriptor
extern struct cpuinfo *cpuinfo_new(void);

// Release the cpuinfo descriptor and all allocated data
extern void cpuinfo_destroy(struct cpuinfo *cip);

/* ========================================================================= */
/* == General Processor Information                                       == */
/* ========================================================================= */

#define CPUINFO_CLASS(N) (((unsigned int)(N)) << 8)

// Processor vendor
enum {
  CPUINFO_VENDOR_UNKNOWN,
  CPUINFO_VENDOR_AMD,
  CPUINFO_VENDOR_IBM,
  CPUINFO_VENDOR_INTEL,
  CPUINFO_VENDOR_MOTOROLA,
  CPUINFO_VENDOR_TRANSMETA,
};

// Get processor vendor ID 
extern int cpuinfo_get_vendor(struct cpuinfo *cip);

// Get processor name
extern const char *cpuinfo_get_model(struct cpuinfo *cip);

// Processor socket
enum {
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
  CPUINFO_SOCKET_S1,
};

// Get processor frequency in MHz
extern int cpuinfo_get_frequency(struct cpuinfo *cip);

// Get processor socket ID
extern int cpuinfo_get_socket(struct cpuinfo *cip);

// Get number of cores per CPU package
extern int cpuinfo_get_cores(struct cpuinfo *cip);

// Get number of threads per CPU core
extern int cpuinfo_get_threads(struct cpuinfo *cip);

/* ========================================================================= */
/* == Processor Caches Information                                        == */
/* ========================================================================= */

enum {
  CPUINFO_CACHE_TYPE_UNKNOWN,
  CPUINFO_CACHE_TYPE_DATA,
  CPUINFO_CACHE_TYPE_CODE,
  CPUINFO_CACHE_TYPE_UNIFIED,
  CPUINFO_CACHE_TYPE_TRACE,
};

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
extern const cpuinfo_cache_t *cpuinfo_get_caches(struct cpuinfo *cip);

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

enum {
  CPUINFO_FEATURE_ARCH	= 0xff00,
  CPUINFO_FEATURE_MASK	= 0x00ff,

  CPUINFO_FEATURE_COMMON = 0,
  CPUINFO_FEATURE_64BIT,
  CPUINFO_FEATURE_SIMD,
  CPUINFO_FEATURE_COMMON_MAX,

  CPUINFO_FEATURE_X86	= CPUINFO_CLASS('X'),
  CPUINFO_FEATURE_X86_CMOV,
  CPUINFO_FEATURE_X86_MMX,
  CPUINFO_FEATURE_X86_SSE,
  CPUINFO_FEATURE_X86_SSE2,
  CPUINFO_FEATURE_X86_SSE3,
  CPUINFO_FEATURE_X86_SSSE3,
  CPUINFO_FEATURE_X86_SSE4,
  CPUINFO_FEATURE_X86_VMX,
  CPUINFO_FEATURE_X86_SVM,
  CPUINFO_FEATURE_X86_LM,
  CPUINFO_FEATURE_X86_LAHF64,
  CPUINFO_FEATURE_X86_BSFCC,
  CPUINFO_FEATURE_X86_MAX,

  CPUINFO_FEATURE_PPC	= CPUINFO_CLASS('P'),
  CPUINFO_FEATURE_PPC_VMX,
  CPUINFO_FEATURE_PPC_FSQRT,
  CPUINFO_FEATURE_PPC_MAX
};

// Returns 0 if CPU supports the specified feature
extern int cpuinfo_has_feature(struct cpuinfo *cip, int feature);

// Utility functions to convert IDs
extern const char *cpuinfo_string_of_vendor(int vendor);
extern const char *cpuinfo_string_of_socket(int socket);
extern const char *cpuinfo_string_of_cache_type(int cache_type);
extern const char *cpuinfo_string_of_feature(int feature);
extern const char *cpuinfo_string_of_feature_detail(int feature);

#endif /* CPUINFO_H */
