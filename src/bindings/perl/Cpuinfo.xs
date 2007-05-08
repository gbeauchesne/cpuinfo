/* -*- c -*-
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

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <string.h>
#include <cpuinfo.h>

static void
register_constant(HV *href, char *name, int val)
{
    newCONSTSUB(href, name, newSViv(val));
}

static void
register_constants(void)
{
    HV *constants = gv_stashpv("Cpuinfo", TRUE);
#define REGISTER_CONSTANT(ID) register_constant(constants, #ID, CPUINFO_##ID)
#include "constants.h"
#undef REGISTER_CONSTANT
}

MODULE = Cpuinfo	PACKAGE = Cpuinfo

BOOT:
    register_constants();

MODULE = Cpuinfo	PACKAGE = Cpuinfo	PREFIX = cpuinfo_

PROTOTYPES: ENABLE

struct cpuinfo *
cpuinfo_new()

void
cpuinfo_DESTROY(cip)
    struct cpuinfo *cip;
CODE:
    cpuinfo_destroy(cip);

int
cpuinfo_get_vendor(cip)
    struct cpuinfo *cip;

const char *
cpuinfo_get_model(cip)
    struct cpuinfo *cip;

int
cpuinfo_get_frequency(cip)
    struct cpuinfo *cip;

int
cpuinfo_get_socket(cip)
    struct cpuinfo *cip;

int
cpuinfo_get_cores(cip)
    struct cpuinfo *cip;

int
cpuinfo_get_threads(cip)
    struct cpuinfo *cip;

void
cpuinfo_get_caches(cip)
    struct cpuinfo *cip;
PREINIT:
    int i;
    const cpuinfo_cache_t *ccp;
PPCODE:
    ccp = cpuinfo_get_caches(cip);
    if (ccp && ccp->count > 0) {
	EXTEND(SP, ccp->count);
	for (i = 0; i < ccp->count; i++) {
	    const cpuinfo_cache_descriptor_t *cdp = &ccp->descriptors[i];
	    HV *rh = newHV();
	    hv_store(rh, "type",  4, newSVnv(cdp->type), 0);
	    hv_store(rh, "level", 5, newSVnv(cdp->level), 0);
	    hv_store(rh, "size",  4, newSVnv(cdp->size), 0);
	    PUSHs(sv_2mortal(newRV((SV *)rh)));
	}
    }

int
cpuinfo_has_feature(cip, feature)
    struct cpuinfo *cip;
    int feature;

const char *
cpuinfo_string_of_vendor(vendor)
    int vendor;

const char *
cpuinfo_string_of_socket(socket)
    int socket;

const char *
cpuinfo_string_of_cache_type(cache_type)
    int cache_type;

const char *
cpuinfo_string_of_feature(feature)
    int feature;

const char *
cpuinfo_string_of_feature_detail(feature)
    int feature;
