/*
 *  cpuinfo-private.h - Private interface
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

#ifndef CPUINFO_PRIVATE_H
#define CPUINFO_PRIVATE_H

#define CPUINFO_FEATURES_SZ_(NAME) \
		(1 + ((CPUINFO_FEATURE_##NAME##_MAX - CPUINFO_FEATURE_##NAME) / 32))

struct cpuinfo {
  int vendor;											// CPU vendor
  char *model;											// CPU model name
  int frequency;										// CPU frequency in MHz
  int socket;											// CPU socket type
  int n_cores;											// Number of CPU cores
  int n_threads;										// Number of threads per CPU core
  cpuinfo_cache_t cache_info;							// Cache descriptors
  uint32_t features[CPUINFO_FEATURES_SZ_(COMMON)];		// Common CPU features
  void *opaque;											// Arch-dependent data
};

typedef struct cpuinfo cpuinfo_t;

/* ========================================================================= */
/* == Lists                                                               == */
/* ========================================================================= */

typedef struct cpuinfo_list {
  void *data;
  struct cpuinfo_list *next;
} *cpuinfo_list_t;

// Clear list
extern int cpuinfo_list_clear(cpuinfo_list_t *lp);

// Insert new element into the list
extern int cpuinfo_list_insert(cpuinfo_list_t *lp, void *ptr, int size);
#define cpuinfo_list_insert(LIST, PTR) (cpuinfo_list_insert)(LIST, PTR, sizeof(*(PTR)))

#define cpuinfo_caches_list_insert(PTR) do {		\
  if (cpuinfo_list_insert(&caches_list, PTR) < 0) {	\
	cpuinfo_list_clear(&caches_list);				\
	return NULL;									\
  }													\
} while (0)

/* ========================================================================= */
/* == General Processor Information (DMI Interface)                       == */
/* ========================================================================= */

// Get processor socket ID
extern int cpuinfo_dmi_get_socket(struct cpuinfo *cip);

// Fill in cache descriptors
extern cpuinfo_list_t cpuinfo_dmi_get_caches(struct cpuinfo *cip);

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

// Accessors for cpuinfo_features[] table
extern int cpuinfo_feature_get_bit(struct cpuinfo *cip, int feature);
extern void cpuinfo_feature_set_bit(struct cpuinfo *cip, int feature);

/* ========================================================================= */
/* == Arch-specific Interface                                             == */
/* ========================================================================= */

// Returns a new cpuinfo descriptor
extern int cpuinfo_arch_new(struct cpuinfo *cip);

// Release the cpuinfo descriptor and all allocated data
extern void cpuinfo_arch_destroy(struct cpuinfo *cip);

// Get processor vendor ID 
extern int cpuinfo_arch_get_vendor(struct cpuinfo *cip);

// Get processor name
extern char *cpuinfo_arch_get_model(struct cpuinfo *cip);

// Get processor frequency in MHz
extern int cpuinfo_arch_get_frequency(struct cpuinfo *cip);

// Get processor socket ID
extern int cpuinfo_arch_get_socket(struct cpuinfo *cip);

// Get number of cores per CPU package
extern int cpuinfo_arch_get_cores(struct cpuinfo *cip);

// Get number of threads per CPU core
extern int cpuinfo_arch_get_threads(struct cpuinfo *cip);

// Get cache information (returns the number of caches detected)
extern cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip);

// Returns features table
extern uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature);

// Returns 0 if CPU supports the specified feature
extern int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature);

#endif /* CPUINFO_PRIVATE_H */
