#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "dma.h"

pthread_t tid[2];
int p_cnt = 0;
void* process_dma(void* arg);
int main()
{
    
    printf("STARTED\n"); 
    dma_init(20); //2^16 = 64KB
    printf ("memory segment is created and initialized \n");

    int i = 0;
    int error;

    while(i < 2){
        error = pthread_create(&(tid[i]), NULL, process_dma, NULL);
        usleep(2000);
        
        if(error != 0){
            printf("\n Thread cannot be created");
        }
        else{
            p_cnt++;
            i++;
        }
        
    }
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);


    dma_print_bitmap();
    dma_print_blocks();
    int aa = dma_give_intfrag();
    printf("\n%d\n", aa);
    printf("ENDED\n"); 
    return (0); 
}

void* process_dma(void* arg){
    void *p;  	
    void *x;
    void *a;
    
    
    p = dma_alloc (250); 
    printf("p is: %p\n", p);
    x = dma_alloc (256);
    printf("x is: %p\n", x);
    dma_free(p);
    p = dma_alloc(240);
    printf("p is: %p\n", p);
    a = dma_alloc(16);
    printf("a is: %p\n", a);
    dma_free(a);
    a = dma_alloc(21);
    printf("a is: %p\n", a);
    //dma_free (p);
    //dma_print_page(2);
    return NULL;
}