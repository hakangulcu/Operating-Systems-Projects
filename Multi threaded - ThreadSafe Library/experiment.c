#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "dma.h"
/** Experiment
 * Usage: ./experiment <filename>
 * filename: Name of the file
 * The input bytes are read from the txt file which is indicated by the user
 */
int main(int argc, char ** argv)
{
    char* filename = argv[1];
    int size = 0;
    printf("Experiment is starting...\n"); 

    int err = dma_init(20);
    if(err != 0){
        printf("There is an error in initialization of the memory block\n");
        return 1;
    }

    FILE *fptr;
    //Open file for counting elements inside
    fptr = fopen(filename, "r");
    if (fptr == NULL){
        printf("file could not be found...\n");
        return 1;
    }
    int b = 0;
    fscanf(fptr, "%d", &b);
    size++;
    while(!feof (fptr)){
        fscanf(fptr, "%d", &b);
        size++;
    }
    fclose(fptr);

    printf("%d ", size);

    void* pointers[size];
    int i = 0;

    //Reopen the file to allocate memory spaces
    fptr = fopen(filename, "r");
    if (fptr == NULL){
        printf("file could not be found...\n");
        return 1;
    }
    b = 0;
    fscanf(fptr, "%d", &b);
    pointers[i] = dma_alloc(b);
    i++;
    //Allocate
    while(!feof (fptr)){
        fscanf(fptr, "%d", &b);
        pointers[i] = dma_alloc(b);
        i++;
    }

    printf("After allocation:\n");
    dma_print_bitmap();
    printf("internal frag: %d", dma_give_intfrag());
    dma_print_blocks();

    //Deallocate
    for(int x = 0; x < size; x++){
        dma_free(pointers[x]);
    }
    printf("After deallocation:\n");
    dma_print_bitmap();
    dma_print_blocks();

    printf("ENDED\n"); 

    fclose(fptr);
    return (0); 
}
