#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char** argv) {
	int fd;
	void* map_base, * virt_addr;
	unsigned long writeval;
	unsigned long long read_result;
	unsigned long long result = 0;
	off_t address, start, end;
	off_t i;


	if (argc < 3) {
		fprintf(stderr, "\nUsage:\t%s {address} {start} {end}\n"
			"\taddress\t: memory address to act upon\n"
			"\tstart\t: decimal offset or 0x{heximal offset}\n"
			"\tend\t: decimal offset or 0x{heximal offset} (default: read 1 byte)\n\n",
			argv[0]);
		exit(1);
	}

	address = strtoul(argv[1], 0, 0);
	start = strtoul(argv[2], 0, 0) + address;
	if (argc >= 4)
		end = strtoul(argv[3], 0, 0) + address;
	else
		end = start;

	if ((fd = open("/dev/mem", O_RDONLY | O_SYNC)) == -1) FATAL;
	printf("/dev/mem opened.\n");
	fflush(stdout);

	/* Map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ, MAP_SHARED, fd, start & ~MAP_MASK);
	if (map_base == (void*)-1) FATAL;
	printf("Memory mapped at address %p.\n\n", map_base);
	fflush(stdout);

	for( i = end; i >= start; i-- ){
		result <<= 8;
		virt_addr = map_base + (i & MAP_MASK);
		read_result = *((unsigned char*)virt_addr);
		printf("Value at address 0x%lX (%p): 0x%llX\n", i, virt_addr, read_result);
		result += read_result; 
		fflush(stdout);
	}
	printf("\nValue at address 0x%lX to 0x%lX: 0x%llX\n", start, end, result);

	if (munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
	return 0;
}
