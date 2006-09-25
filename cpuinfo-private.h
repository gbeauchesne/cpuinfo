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

/* ========================================================================= */
/* == General Processor Information (DMI Interface)                       == */
/* ========================================================================= */

// Get processor socket ID
extern int cpuinfo_dmi_get_socket(void);

// Get cache information (initialize with iter = 0, returns the
// iteration number or -1 if no more information available)
extern int cpuinfo_dmi_get_cache(int iter, cpuinfo_cache_t *cip);

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

// Accessors for cpuinfo_features[] table
extern int cpuinfo_feature_get_bit(int feature);
extern void cpuinfo_feature_set_bit(int feature);

#endif /* CPUINFO_PRIVATE_H */
