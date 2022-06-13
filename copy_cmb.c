#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CP_SIZE (1 << 29)
#define MAP_SIZE 16384UL

#define CMB_ADDR 0xC0000000

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

int main(int argc, char** argv){
    if(argc < 3){
        fprintf(stderr, "%s file {c, p}\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd, tfd;

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    if((tfd = open(argv[1], O_RDWR|O_CREAT, 0644)) == -1) FATAL;
    ftruncate(tfd, CP_SIZE);

    u_int64_t* mem;
    u_int64_t* tmem;

    size_t len = CP_SIZE / MAP_SIZE;
    int i = 0;
    while(len--){
        mem = (u_int64_t*) mmap(NULL, MAP_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, CMB_ADDR + i * MAP_SIZE);
        if(mem == MAP_FAILED) FATAL;
        tmem = (u_int64_t*) mmap(NULL, MAP_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, tfd, i * MAP_SIZE);
        if(tmem == MAP_FAILED) FATAL;

        u_int64_t* dest = tmem;
        u_int64_t* src = mem;

        size_t page = MAP_SIZE / sizeof(u_int64_t);
        while(page--){
            if(argv[2][0] == 'c')
                *(dest++) = *(src++);
            else if(argv[2][0] == 'p')
                *(src++) = *(dest++);
            else
                FATAL;
            
            if(page % 1024 == 0) usleep(1);
        }
        i++;
        printf("Progress: %f%%\r", (double)i * MAP_SIZE / CP_SIZE * 100);

        munmap(mem, MAP_SIZE);
        munmap(tmem, MAP_SIZE);
    }

    close(fd);
    close(tfd);

    return 0;
}
