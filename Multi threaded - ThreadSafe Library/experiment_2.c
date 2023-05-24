#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "dma.h"
/** Experiment_2 takes 3 inputs
 * Usage: ./experiment_2 <array_size> <size> <max_byte>
 * array_size: Size of the array for bytes
 * size: size of the memory (it needs to be between 14 and 22)
 * max_byte: max byte which the allocation can take
 * Random numbers for byte is generated in this program according to inputs
 * array_size * max_byte <= 2^size
 */
int main(int argc, char ** argv) {
    int arr_size = atoi(argv[1]);
    int seg_size = atoi(argv[2]);
    int max = atoi(argv[3]);
    struct timeval curr_time, end_time;
    
    if( arr_size * max > pow(2,seg_size)){
        printf("Array size * max <= 2^segment_size");
        return 1;
    }
    srand(time(0));
    void *pointers[arr_size];
    for(int i = 0; i < arr_size; i++){
        
        pointers[i] = NULL;
    }
    //Initialization of memory
    int err = dma_init(seg_size);
    if(err != 0){
        printf("There is an error in initialization of the memory block\n");
        return 1;
    }
    for(int i = 0; i < arr_size; i++){

        int x = rand() % max; //Selects number between 0 and max_byte
        gettimeofday(&curr_time, NULL);
        pointers[i] = dma_alloc(x);
        gettimeofday(&end_time, NULL);
        printf("Alloc %d completed in %ld microseconds\n", i, ((end_time.tv_sec - curr_time.tv_sec) * 1000000 + (end_time.tv_usec - curr_time.tv_usec)));
    }
    dma_print_blocks();
    int frag = dma_give_intfrag();
    printf("internal frag = %d\n", frag);
    for(int i = 0; i < arr_size; i++){
        dma_free(pointers[i]);
    }
    
    return 0;
}