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
#include <time.h>
#include <sys/time.h>
  
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
//#define MAP_SIZE 16384UL
#define MAP_SIZE (1 << 29)
#define MAP_MASK (MAP_SIZE - 1)
#define BAR_SIZE (1 << 29) // 0x2000 0000 (512MB)
#define LOOP (MAP_SIZE / 8)

#define CMB_ADDR 0XC0000000

u_int64_t random_int64(){
	u_int64_t result = 0;
	for(int i = 0; i < sizeof(u_int64_t) * 8; i++){
		result = (result << 1) + rand()%2;
	}
	return result;
}

int main(int argc, char **argv) {
    int fd;
    void *cmb_base, *ref_base, *read_base; 

    u_int64_t *dest, *src;

    int *rand_off;
    u_int64_t *randval;
	
    int interval = 3736;
    int i;

    struct timespec start, end;
    u_int64_t delta_us;

	srand(time(NULL));

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    printf("/dev/mem opened.\n"); 
    fflush(stdout);

    // Map CMB, reference, raed
    cmb_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CMB_ADDR);
    if(cmb_base == (void *) -1) FATAL;
    ref_base = calloc(1, MAP_SIZE);
    if(ref_base == 0) FATAL;
    read_base = calloc(1, MAP_SIZE);
    if(read_base == 0) FATAL;

    // Clear CMB
    printf("Clear CMB\n");
    dest = cmb_base;
    i = LOOP;
    while(i--){
       *(dest++) = 0; 
        if(i % interval == 0) usleep(1);
    }

    // Create Random Addr and Value
    printf("Generating Random Address and Value\n");
    randval = (u_int64_t*) malloc(sizeof(u_int64_t) * LOOP);
    if(randval == 0) FATAL;
    rand_off = (int*) malloc(sizeof(int) * LOOP); 
    if(rand_off == 0) FATAL;
    for(int i = 0; i < LOOP; i++){
        rand_off[i] = rand() % LOOP;
        randval[i] = random_int64();
    }

    // Write Test
    printf("Write Test\n");
    printf("Sleep 1us for every %d writes\n", interval);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	for(int i = 0; i < LOOP; i++){
        dest = cmb_base;
        dest += rand_off[i];

        *dest = randval[i];
        if(i % interval == 0) usleep(1);        
	}
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Write total latency = %ldus, avarge latency = %fus\n", delta_us, (double)delta_us/LOOP);

    // Read Test
    printf("Read Test\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	for(int i = 0; i < LOOP; i++){
        src = cmb_base;
        src += rand_off[i];
        dest = read_base;
        dest += rand_off[i];

        *dest = *src;
	}
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Read total latency = %ldus, avarge latency = %fus\n", delta_us, (double)delta_us/LOOP);
    
    // Compare
    printf("Generating Expected Result\n");
	for(int i = 0; i < LOOP; i++){
        dest = ref_base;
        dest += rand_off[i];
        *dest = randval[i];
	}
    printf("Comparing Results\n");
    i = LOOP;
    dest = cmb_base;
    src = ref_base;
    int err = 0;
    while(i--){
        if(*(dest++) != *(src++)){
            err++;
        }    
    }
    printf("Error rate = %f%%( %d / %d )\n", (double)err * 100 / LOOP, err, LOOP);

	if(munmap(cmb_base, MAP_SIZE) == -1) FATAL;
    free(randval);
    free(rand_off);
    close(fd);

    return 0;
}
