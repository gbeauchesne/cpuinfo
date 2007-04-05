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

struct cpuinfo {
  int vendor;					// CPU vendor
  const char *model;			// CPU model name
  int frequency;				// CPU frequency in MHz
  int socket;					// CPU socket type
  int n_cores;					// Number of CPU cores
  int n_threads;				// Number of threads per CPU core
  cpuinfo_cache_t cache_info;	// Cache descriptors
  uint32_t *common_features;	// Common CPU features table
  uint32_t *x86_features;		// X86 features table
  uint32_t *ppc_features;		// PowerPC features table
};

typedef struct cpuinfo cpuinfo_t;

/* ========================================================================= */
/* == General Processor Information (DMI Interface)                       == */
/* ========================================================================= */

// Get processor socket ID
extern int cpuinfo_dmi_get_socket(struct cpuinfo *cip);

// Fill in cache descriptors
extern void cpuinfo_dmi_get_caches(struct cpuinfo *cip);

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

// Accessors for cpuinfo_features[] table
extern int cpuinfo_feature_get_bit(struct cpuinfo *cip, int feature);
extern void cpuinfo_feature_set_bit(struct cpuinfo *cip, int feature);

#endif /* CPUINFO_PRIVATE_H */
