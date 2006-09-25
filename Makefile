AR = ar
CC = gcc
CFLAGS = -O2

libcpuinfo_a = libcpuinfo.a
libcpuinfo_a_SOURCES = cpuinfo-common.c cpuinfo-x86.c cpuinfo-ppc.c cpuinfo-dmi.c
libcpuinfo_a_OBJECTS = $(libcpuinfo_a_SOURCES:%.c=%.o)

cpuinfo_SOURCES = cpuinfo.c
cpuinfo_OBJECTS = $(cpuinfo_SOURCES:%.c=%.o)

PROGS = cpuinfo

all: $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f $(libcpuinfo_a) $(libcpuinfo_a_OBJECTS)
	rm -f $(cpuinfo_OBJECTS)

cpuinfo: $(cpuinfo_OBJECTS) $(libcpuinfo_a)
	$(CC) -o $@ $(cpuinfo_OBJECTS) $(libcpuinfo_a) $(LDFLAGS)

$(libcpuinfo_a): $(libcpuinfo_a_OBJECTS)
	$(AR) rc $@ $(libcpuinfo_a_OBJECTS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
