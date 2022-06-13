#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define CP_SIZE (1 << 29)
#define MAP_SIZE (1 << 29)

#define CMB_ADDR 0xC0000000

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

// Generate random 64-bit(8B) number #Later
u_int64_t rand64(){
    u_int64_t x = 0;
    for(int i = 0; i < sizeof(u_int64_t) * 8; i++){
        x = (x << 1) | (rand() % 2);
    }
    return x;
}

int main(int argc, char** argv){
    // Create Random malloc
    printf("Generating Random Number Dataset\n");
	srand(time(NULL));
    u_int64_t* rand = (u_int64_t*) malloc(MAP_SIZE);
    if (rand == 0) FATAL;
    size_t i = MAP_SIZE / sizeof(u_int64_t);
    u_int64_t* dst = rand;
    while(i--){
        *(dst++) = rand64();
    }

    // Create another empty malloc
    u_int64_t* empty = (u_int64_t*) malloc(MAP_SIZE);
    if (empty == 0) FATAL;

    // Open CMB
    int fd;
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;

    u_int64_t* cmb = (u_int64_t*) mmap(NULL, MAP_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, CMB_ADDR);
    if(cmb == MAP_FAILED) FATAL;

    u_int64_t *dest, *src;
    u_int64_t delta_us;
    struct timespec start, end;

    // Write Test
    printf("Write Test\n");
    int interval = 3740;
    printf("Sleep 1us for every %d writes\n", interval);
    dest = cmb;
    src = rand;
    i = MAP_SIZE / sizeof(u_int64_t);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    while(i--){
        *(dest++) = *(src++);        
        if(i % interval == 0) usleep(1);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Write total latency = %ldus, avarge latency = %fus\n", delta_us, (double)delta_us/(MAP_SIZE / sizeof(u_int64_t)));

    // Read Test
    printf("Read Test\n");
    dest = empty;
    src = cmb;
    i = MAP_SIZE / sizeof(u_int64_t);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    while(i--){
        *(dest++) = *(src++);        
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Read total latency = %ldus, avarge latency = %fus\n", delta_us, (double)delta_us/(MAP_SIZE / sizeof(u_int64_t)));

    // Compare
    printf("Comapring Results\n");
    int err = 0;
    dest = rand;
    src = empty;
    i = MAP_SIZE / sizeof(u_int64_t);
    while(i--){
        if(*(dest++) != *(src++)){
            err++;
        }    
    }

    printf("Error rate = %f%%( %d / %ld )\n", (double)err * 100 / (MAP_SIZE / sizeof(u_int64_t)), err, (MAP_SIZE / sizeof(u_int64_t)));

	if(munmap(cmb, MAP_SIZE) == -1) FATAL;
    free(rand);
    free(empty);
    close(fd);

    return 0;
}
