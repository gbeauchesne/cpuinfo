/*
 *  cpuinfo-dmi.c - Processor identification code, SMBIOS specific
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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "cpuinfo.h"

#define DEBUG 1
#include "debug.h"

#if defined __i386__ || defined __x86_64__

#define DEFAULT_MEM_DEV "/dev/mem"

void *mem_chunk(uint32_t base, uint32_t len, const char *devmem)
{
  int fd;
  void *p;
  void *mmp;
  off_t mmoffset;

  if ((fd = open(devmem, O_RDONLY)) < 0)
	return NULL;

  if ((p = malloc(len)) == NULL)
	return NULL;

  mmoffset = base % getpagesize();
  mmp = mmap(0, mmoffset + len, PROT_READ, MAP_SHARED, fd, base - mmoffset);
  if (mmp == MAP_FAILED) {
	free(p);
	return NULL;
  }

  memcpy(p, (uint8_t *)mmp + mmoffset, len);
  munmap(mmp, mmoffset + len);
  close(fd);
  return p;
}

struct dmi_header {
  uint8_t	type;
  uint8_t	length;
  uint16_t	handle;
};

typedef int (*dmi_decode)(uint8_t *data, void *user_data);

static int decode_handle(uint32_t base, int len, int num, dmi_decode decode, void *user_data)
{
  uint8_t *buf;
  uint8_t *data;
  int i = 0;
  int ret = 0;

  if ((buf = mem_chunk(base, len, DEFAULT_MEM_DEV)) == NULL)
	return 0;

  data = buf;
  while (i < num && data + sizeof(struct dmi_header) <= buf + len) {
	uint8_t *next;
	struct dmi_header *dm = (struct dmi_header *)data;

	/* look for the next handle */
	next = data + dm->length;
	while (next - buf + 1 < len && (next[0] != 0 || next[1] != 0))
	  next++;
	next += 2;
	if (next - buf <= len) {
	  int r = decode(data, user_data);
	  if (r < 0)
		break;
	  ret += r;
	}
	else {
	  ret = 0; /* TRUNCATED */
	  break;
	}
	data = next;
	i++;
  }

  free(buf);
  return ret;
}

static int dmi_detect(dmi_decode decode, void *user_data)
{
  uint8_t *buf;
  if ((buf = mem_chunk(0xf0000, 0x10000, DEFAULT_MEM_DEV)) == NULL)
	return -1;

  int ret = 0;
  uint32_t fp;
  for (fp = 0; fp <= 0xfff0; fp += 16) {
	if (memcmp(buf + fp, "_DMI_", 5) == 0) {
	  uint8_t *p = buf + fp;
	  uint16_t num = (p[13] << 8) | p[12];
	  uint16_t len = (p[7] << 8) | p[6];
	  uint32_t base = (p[11] << 24) | (p[10] << 16) | (p[9] << 8) | p[8];

	  ret = decode_handle(base, len, num, decode, user_data);
	  break;
	}
  }

  free(buf);
  return ret;
}

static int dmi_socket(uint8_t *data, int *p_socket)
{
  struct dmi_header *dm = (struct dmi_header *)data;

  if (dm->type == 4) {
	int socket = CPUINFO_SOCKET_UNKNOWN;
	D(bug("* dmi_socket %02x\n", data[0x19]));
	switch (data[0x19]) {
	case 0x0f:	socket = CPUINFO_SOCKET_478;	break;
	case 0x10:	socket = CPUINFO_SOCKET_754;	break;
	case 0x11:	socket = CPUINFO_SOCKET_940;	break;
	case 0x12:	socket = CPUINFO_SOCKET_939;	break;
	case 0x13:	socket = CPUINFO_SOCKET_604;	break;
	case 0x14:	socket = CPUINFO_SOCKET_771;	break;
	case 0x15:	socket = CPUINFO_SOCKET_775;	break;
	}
	if (p_socket)
	  *p_socket = socket;
	return -1;
  }

  return 0;
}

// Get processor socket ID
int cpuinfo_dmi_get_socket(void)
{
  static int socket = -1;

  if (socket < 0) {
	if (dmi_detect((dmi_decode)dmi_socket, &socket) < 0)
	  socket = CPUINFO_SOCKET_UNKNOWN;
  }

  return socket;
}

static int dmi_cache_handle(uint8_t *data, uint16_t *p_handle)
{
  struct dmi_header *dm = (struct dmi_header *)data;

  if (p_handle == NULL)
	return -1;

  if (dm->type == 4) {
	int ofs = *p_handle;
	*p_handle = (data[ofs + 1] << 8) | data[ofs];
	D(bug("* dmi_cache_handle %04x\n", *p_handle));
	return -1;
  }

  return 0;
}

static int dmi_cache_info(uint8_t *data, cpuinfo_cache_t *cip)
{
  struct dmi_header *dm = (struct dmi_header *)data;

  if (dm->type == 7 && dm->handle == cip->type) {
	int cache_type = data[0x11];
	switch (cache_type) {
	case 3: cip->type = CPUINFO_CACHE_TYPE_CODE; break;
	case 4: cip->type = CPUINFO_CACHE_TYPE_DATA; break;
	case 5: cip->type = CPUINFO_CACHE_TYPE_UNIFIED; break;
	default: cip->type = CPUINFO_CACHE_TYPE_UNKNOWN; break;
	}
	uint16_t installed_size = (data[0x0a] << 8) | data[0x09];
	D(bug("* dmi_cache_info %02x, %04x\n", cache_type, installed_size));
	cip->size = installed_size & 0x7fff;		// 1K granularity (default)
	if (installed_size & 0x8000)
	  cip->size *= 64;							// 64K granularity
  }

  return 0;
}

static int get_cache(int ofs, cpuinfo_cache_t *cip)
{
  uint16_t handle = ofs;
  if (dmi_detect((dmi_decode)dmi_cache_handle, &handle) < 0)
	return -1;
  if (handle == 0xffff)
	return -1;
  cip->type = handle;
  if (dmi_detect((dmi_decode)dmi_cache_info, cip) < 0)
	return -1;
  return 0;
}

// Get cache information (initialize with iter = 0, returns the
// iteration number or -1 if no more information available)
int cpuinfo_dmi_get_cache(int iter, cpuinfo_cache_t *cip)
{
#define N_CACHE_DESCRIPTORS 3 /* 3 handles at most, SMBIOS 2.5 */
  static cpuinfo_cache_t ci[N_CACHE_DESCRIPTORS];
  static int ci_count = -1;

  if (ci_count < 0) {
	ci_count = 0;
	if (get_cache(0x1a, &ci[ci_count]) == 0) {
	  ci[ci_count].level = 1;
	  ci_count++;
	}
	if (get_cache(0x1c, &ci[ci_count]) == 0) {
	  ci[ci_count].level = 2;
	  ci_count++;
	}
	if (get_cache(0x1e, &ci[ci_count]) == 0) {
	  ci[ci_count].level = 3;
	  ci_count++;
	}
  }

  if (iter < 0 || iter >= ci_count)
	return -1;

  if (cip)
	*cip = ci[iter];

  if (++iter == N_CACHE_DESCRIPTORS)
	return -1;

  return iter;
}

#endif
