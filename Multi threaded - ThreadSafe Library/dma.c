#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "dma.h"
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

// Define mutex locks
pthread_mutex_t segment_lock;

// Define variables
#define MAX_M 22
#define MIN_M 14
#define word_size 8
#define reserved 256


size_t segSize;
void *start_ptr;
unsigned long int *alloc_start_addr;
int bitmap_size, bitmap_reserved, block_reserved;
int frag_amount = 0;
int cnt = 0;

int dma_init(int m)
{
    //size_t pagesize = getpagesize();
    //printf("System page size: %zu bytes\n", pagesize);
    
    // Segment size control min 16KB max 4MB
    if (m < MIN_M || m > MAX_M)
    {
        printf("Heap size should in between 16KB and 4MB \n");
        pthread_mutex_unlock(&segment_lock);
        return -1;
    }

    // convert m to 2^pow
    segSize = pow(2, m);

    start_ptr = mmap(NULL, segSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (start_ptr == MAP_FAILED)
    {
        printf("mmap failed\n");
        return -1;
    }
    printf("start_ptr : %p\n", start_ptr);
    //printf("m : %d segSize: %d\n", m, segSize);

    // Init bitmap
    bitmap_size = pow(2, (m - 3));

    for (int i = 0; i < bitmap_size; i++)
    {
        ((unsigned long int *)start_ptr)[i] = 1;
    }

    bitmap_reserved = pow(2, (m - 6)) / word_size;
    block_reserved = reserved / word_size;
    ((unsigned long int *)start_ptr)[0] = 0;
    for(int i = 2; i < bitmap_reserved; i++)
        ((unsigned long int *)start_ptr)[i] = 0;
    ((unsigned long int *)start_ptr)[bitmap_reserved] = 0;
    for(int i = bitmap_reserved + 2; i < bitmap_reserved + block_reserved; i++){
        ((unsigned long int *)start_ptr)[i] = 0;
    }
    //Allocation beginning address = &(((unsigned long int*)start_ptr)[bitmap_reserved+reserved])
    
    // calculate the alloc start addr = bitmap + reserved area
    alloc_start_addr = &(((unsigned long int*)start_ptr)[bitmap_size+reserved]);
    
    // initialize mutex
    pthread_mutex_init(&segment_lock, NULL);
    return (0);
}

void *dma_alloc(int size)
{
    // lock the allocation
    pthread_mutex_lock(&segment_lock);
    struct timeval curr_time, end_time;
    gettimeofday(&curr_time, NULL);
    //while( )
    //printf("current time is: %ld\n", curr_time.tv_usec);
    
    if (size > segSize)
    {
        printf("You cannot allocate larger than the original heap... u idiot \n");
        pthread_mutex_unlock(&segment_lock);
        gettimeofday(&end_time, NULL);
        printf("Alloc completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
        return NULL;
    }

    // can allocate a multiple of 16 bytes
    //printf("size before : %d\n", size);
    int alloc_size;
    if (size % 16 != 0)
    {
        alloc_size = ((size / 16) + 1) * 16;
    } else {
        alloc_size = size;
    }
    //printf("size after : %d\n", alloc_size);
    frag_amount += alloc_size - size;

    int isStarted = 0;
    int start_idx = 0;
    int end_idx = 0;
    for( int i = 0; i < bitmap_size - 1; i+=2 ){
        if (((unsigned long int *)start_ptr)[i] == 1) {
            if (((unsigned long int *)start_ptr)[i+1] == 1 && !isStarted){ //empty area
                isStarted = 1;
                start_idx = i;
            }
            else if(((unsigned long int *)start_ptr)[i+1] == 1){
                
                if( ((i - start_idx) * 8) == alloc_size ){
                    ((unsigned long int *)start_ptr)[start_idx] = 0;
                    for( int x = start_idx+2; x < i; x++ )
                        ((unsigned long int *)start_ptr)[x] = 0;
                    int alloc_addr = start_idx - bitmap_reserved - block_reserved;
                    pthread_mutex_unlock(&segment_lock);
                    gettimeofday(&end_time, NULL);
                    printf("Alloc completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
                    //printf("end time is: %ld\n", curr_time.tv_usec);
                    return &(alloc_start_addr[alloc_addr]);
                }
            }
        } else {
            if( ((unsigned long int *)start_ptr)[i+1] == 1 && isStarted){
                end_idx = i;
                isStarted = 0;
                if( ((end_idx - start_idx) * 8) == alloc_size ){
                    
                    ((unsigned long int *)start_ptr)[start_idx] = 0;
                    for( int x = start_idx+2; x < end_idx; x++ )
                        ((unsigned long int *)start_ptr)[x] = 0;
                    int alloc_addr = start_idx - bitmap_reserved - block_reserved;
                    pthread_mutex_unlock(&segment_lock);
                    gettimeofday(&end_time, NULL);
                    printf("Alloc completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
                    return &(alloc_start_addr[alloc_addr]);
                }
            }
        }
    }

    // release the lock
    pthread_mutex_unlock(&segment_lock);
    gettimeofday(&end_time, NULL);
    printf("Alloc completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));

    return (NULL);
}

void dma_free(void *p)
{
    pthread_mutex_lock(&segment_lock);
    struct timeval curr_time, end_time;
    gettimeofday(&curr_time, NULL);
    
    if( p == NULL ){
        gettimeofday(&end_time, NULL);
        
        printf("Free completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
        pthread_mutex_unlock(&segment_lock);
        return;
    }
    
    //implementation comes here
    //printf("%p\n", ((unsigned long int*)p));
    //printf("%p\n", ((unsigned long int*)start_ptr));
    int bitmap_loc = ((unsigned long int*)p) - alloc_start_addr;
    bitmap_loc += block_reserved + bitmap_reserved;
    //bitmap_loc = bitmap_loc - alloc_start_addr;
    //printf("%d", bitmap_loc);
    ((unsigned long int *)start_ptr)[bitmap_loc] = 1;
    size_t size = 2;
    for(int i = bitmap_loc+2; i < bitmap_size-1; i+=2){
        if(((unsigned long int *)start_ptr)[i] == 0 && ((unsigned long int *)start_ptr)[i+1] == 0){
            ((unsigned long int *)start_ptr)[i] = 1;
            ((unsigned long int *)start_ptr)[i+1] = 1;
            size += 2;
        } else {
            break;
        }
    }
    int success = munmap(p, size);
    if(!success){
        printf("Deallocation failed!\n");
        pthread_mutex_unlock(&segment_lock);
        gettimeofday(&end_time, NULL);
        printf("Free completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
        return;
    } else {
        printf("Deallocation successful!\n");
        pthread_mutex_unlock(&segment_lock);
        gettimeofday(&end_time, NULL);
        printf("Free completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
        return;
    }

    
    
    //destroy the lock
    pthread_mutex_unlock(&segment_lock);
    gettimeofday(&end_time, NULL);
    
    printf("Free completed in %ld microseconds\n", ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));


}

void dma_print_bitmap()
{
    int a;
    int sequence = 0;
    
    // directly access to the heap start address
    unsigned long int *p = (unsigned long int *)start_ptr;
    int new_size = bitmap_size;
    // retrieve bitmap items
    printf(" ");
    for (int i = 0; i < new_size; i++)
    {
        a = p[i];
        // printf("bitmap %d th : %d ", i, a);
        printf("%d ", a);
        sequence = sequence + 1;
        if (sequence % 64 == 0) {printf("\n");}
        if (sequence % 8 == 0) {printf(" ");}
    }
}

void dma_print_blocks(){

    unsigned long int *p = (unsigned long int *)start_ptr;
    int start_idx = 0;
    unsigned long int* end_addr;
    int end_idx = 0;
    int alloc_lock = 0;
    int free_lock = 0;
    int block_size = 0;

    printf("\n");
    for (int i = 0; i < bitmap_size - 1; i+=2){
        //bSize = 010000 111 =  6 full block size
        if(p[i] == 1){ //1x
            if(p[i+1] == 1){ //11
                if(alloc_lock == 1){
                    //pattern başı
                    end_idx = start_idx * word_size;
                    end_addr = &(start_ptr[end_idx]);
                    block_size += 2;
                    block_size *= 8;
                    printf("A, %p, 0x%x (%d) \n", end_addr, block_size, block_size);
                    alloc_lock = 0;
                    free_lock = 1;
                    start_idx = i;
                    block_size = 0;
                }
                //pattern devamı
                block_size += 2;
                free_lock = 1;
                //printf("start_idx: %d\n", start_idx);
            }
        }
        else { //0x
            if(p[i+1] == 1){ //01 is the start
                if(free_lock == 1){
                    free_lock= 0;
                    end_idx = start_idx * word_size;
                    end_addr = &(start_ptr[end_idx]);
                    start_idx = i;
                    block_size *= 8;
                    printf("F, %p, 0x%x (%d) \n", end_addr, block_size, block_size);
                    block_size = 0;
                    alloc_lock = 1;
                }
                else if(alloc_lock == 1){
                    end_idx = start_idx * word_size;
                    end_addr = &(start_ptr[end_idx]);
                    block_size += 2;
                    block_size *= 8;
                    printf("A, %p, 0x%x (%d) \n", end_addr, block_size, block_size);
                    start_idx = i;
                    block_size = 0;
                }
                else{
                    
                    alloc_lock = 1;
                }
                //printf("start_idx: %d alloc_blok_adr = %#016x\n", start_idx, alloc_block_addr);
                
            }
            else{ //00 part = calc allocated size
                block_size += 2;
                //printf("start_idx: %d\n", start_idx);
            }
        }
    }

    if(alloc_lock == 1){
        end_idx = start_idx * word_size;
        end_addr = &(start_ptr[end_idx]);
        block_size *= 8;
        printf("A, %p, 0x%x (%d) \n", end_addr, block_size, block_size);
    }
    else if(free_lock == 1){
        end_idx = start_idx* word_size;
        end_addr = &(start_ptr[end_idx]);
        block_size *= 8;
        printf("F, %p, 0x%x (%d) \n", end_addr, block_size, block_size);
    }
    //int aa = pow(2, 20) - pow(2,14) - pow(2,8);
    //printf("\n%d",aa);
    printf("\n");
}

int dma_give_intfrag(){
    return frag_amount;
}

void dma_print_page(int pno)
{
    if (pno > segSize / word_size || pno < 0)
    {
        printf("Please enter a valid page no");
    }
    else
    {
        unsigned long int* a;
        int sequence = 0;
        a = (unsigned long int *)start_ptr;

        printf("\n");
        printf("page %d is printing now...\n", pno);
        for (int i = (pno * word_size); i < segSize; i+word_size)
        {
            // get the page addr
            a = &(a[i]);
            // retrieve page
            printf("%ln", a);
            
            sequence = sequence + 8;
            if (sequence % 64 == 0)
            {
                printf("\n");
            }
        }
    }
}