#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
char *ALG;        // algorithm
int Q;            // = 2;          // time quantum
int T1;           // service time of device 1
int T2;           // service time of device 2
char *burst_dist; // fix,uniform or exponential
int burstlen;     // = 5;   // burst lenght
int min_burst;    // = 2;  // min burst
int max_burst;    // = 20; // max burst
double p0;        // prob 1
double p1;        // prob 2
double p2;        // prob 3
double pg;        // = 0.9;    // prob of creating new thread
int MAXP;         // = 5;       // max thread count 1<= MAXP <= 50
int ALLP;         // = 8;       // all thread count 1<= ALLP <= 1000
int OUTMODE;      // choose output
int processCount;
int remainingProcessCount;
int numberOfProcessesAtQueue = 0;
int algo = 0;
int nextToQueue = 0;
int CPUBusy = 0;
struct timeval start, end;
int allFinished = 0;
/*
ALG = atoi(argv[1]);        // algorithm
     Q = atoi(argv[2]);          // time quantum
     T1 = atoi(argv[3]);         // service time of device 1
    T2 = atoi(argv[4]);         // service time of device 2
     burst_dist = atoi(argv[5]); // fix,uniform or exponential
     burstlen = atoi(argv[6]);   // burst lenght
     min_burst = atoi(argv[7]);  // min burst
     max_burst = atoi(argv[8]);  // max burst
     p0 = atoi(argv[9]);         // prob 1
    p1 = atoi(argv[10]);        // prob 2
    p2 = atoi(argv[11]);        // prob 3
    pg = atoi(argv[12]);        // prob of creating new thread
    MAXP = atoi(argv[13]);      // max thread count 1<= MAXP <= 50
    ALLP = atoi(argv[14]);      // all thread count 1<= ALLP <= 1000
    OUTMODE = atoi(argv[15]);   */

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;

struct timeval current_time;
struct arg
{

    int pid;
    int thread_id;
    struct Queue *que;
};

struct PCB
{
    struct PCB *next;
    struct PCB *prev;
    int pid;
    int thread_id;
    int state; // 0 waiting 1 running 2 ready
    int next_CPU_burst_length;
    int remaining_CPU_burst_length; // for rr
    int number_of_CPU_bursts_executed;
    int time_spent_ready_list;
    int count_io_device1;
    int count_io_device2;
    int startTime;
    int finishTime;
    int arrivalTime;
    int turnoverTime;
    int total_time_spent_CPU;
};

struct arg2
{
    int pid;
    struct Queue *que;
};

/**
 * @brief
 * createNode();
 *
 */

struct PCB *createNode(int pid,
                       int thread_id,
                       int state,
                       int next_CPU_burst_length,
                       int remaining_CPU_burst_length,
                       int number_of_CPU_bursts_executed,
                       int time_spent_ready_list,
                       int count_io_device1,
                       int count_io_device2,
                       int startTime,
                       int finishTime,
                       int arrivalTime,
                       int turnoverTime,
                       int total_time_spent_CPU)
{
    struct PCB *node = (struct PCB *)malloc(sizeof(struct PCB));

    node->pid = pid;
    node->thread_id = thread_id;
    node->state = state;
    node->next_CPU_burst_length = next_CPU_burst_length;
    node->remaining_CPU_burst_length = remaining_CPU_burst_length;
    node->number_of_CPU_bursts_executed = number_of_CPU_bursts_executed;
    node->time_spent_ready_list = time_spent_ready_list;
    node->count_io_device1 = count_io_device1;
    node->count_io_device2 = count_io_device2;
    node->startTime = startTime;
    node->finishTime = finishTime;
    node->arrivalTime = arrivalTime;
    node->turnoverTime = turnoverTime;
    node->total_time_spent_CPU = total_time_spent_CPU;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

struct Queue
{
    struct PCB *endNode;
    struct PCB *startNode;
    // pthread_cond_t  cv;// = PTHREAD_COND_INITIALIZER;
    // cnd_t cv;
    pthread_cond_t cvReadyQueue; // = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t lock;
};

struct Queue *createQueue()
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->startNode = NULL;
    queue->endNode = NULL;
    pthread_cond_init(&(queue->cvReadyQueue), NULL);
    // queue->cv = PTHREAD_COND_INITIALIZER;
    return queue;
}

void enqueue(struct Queue *queue, struct PCB *node)
{
    if (queue->startNode == NULL) // && queue->endNode == NULL)
    {
        queue->startNode = node;
        queue->endNode = node;
    }
    else if (queue->startNode != NULL && queue->endNode != NULL)
    {
        queue->endNode->next = node;
        node->prev = queue->endNode;
        queue->endNode = node;
    }
    // printf("ben enguaelendim %d \n ", node->pid);
}

void dequeue(struct Queue *queue)
{

    if (queue->startNode == NULL)
    {
        return;
    }
    struct PCB *tempNode = queue->startNode;
    if (queue->startNode == queue->endNode)
    {
        // printf("ben dequeue %d \n ", tempNode->pid);
        queue->startNode = NULL;
        queue->endNode = NULL;
        free(tempNode);
    }
    else
    {
        queue->startNode = queue->startNode->next;
        queue->startNode->prev = NULL;
        // printf("ben dequeue %d \n ", tempNode->pid);
        free(tempNode);
    }
}
void dequeue1(struct Queue *queue)
{

    if (queue->startNode == NULL)
    {
        return;
    }
    struct PCB *tempNode = queue->startNode;
    if (queue->startNode == queue->endNode)
    {
        // printf("ben dequeue %d \n ", tempNode->pid);
        queue->startNode = NULL;
        queue->endNode = NULL;
        // free(tempNode);
    }
    else
    {
        queue->startNode = queue->startNode->next;
        queue->startNode->prev = NULL;
        // printf("ben dequeue %d \n ", tempNode->pid);
        //  free(tempNode);
    }
}

int calculateNextTime()
{
    int flag = 0;
    int nextCpuBurst = 0;
    while (flag == 0)
    {
        double lambda = 1 / burstlen;
        double u = ((double)rand() / (RAND_MAX));
        // nextCpuBurst = ((-1) * (log(u)))/lambda;
        nextCpuBurst = 5;
        if (nextCpuBurst >= min_burst && nextCpuBurst <= max_burst)
        {
            flag = 1;
        }
    }
    return nextCpuBurst;
}
void *addThreadToQueue(void *arg_ptr)
{
    int pid = ((struct arg *)arg_ptr)->pid;
    while (pid != nextToQueue)
    { // en baştakini seçiyoruz sırayla
    }
    // printf("sdklfojdsjkfds %d \n", numberOfProcessesAtQueue);
    numberOfProcessesAtQueue++;
    int nextCpuBurstTime = calculateNextTime();
    int thread_id = ((struct arg *)arg_ptr)->thread_id;

    int state = 2;
    int remaining_CPU_burst_length = burstlen;
    int number_of_CPU_bursts_executed = 0;
    int time_spent_ready_list = 0;
    int count_io_device1 = 0;
    int count_io_device2 = 0;
    int startTime = 0;
    int finishTime = 0;
    int arrivalTime = 0;
    int turnoverTime = 0;
    int total_time_spent_CPU = 0;
    struct Queue *queue = ((struct arg *)arg_ptr)->que; // BURAYI DIREK ENQUEUEYE DE KOYABILIRDIK TEKRAR ESITLEMEKTENSE
    struct PCB *PCB = createNode(pid, thread_id, state, nextCpuBurstTime, remaining_CPU_burst_length, number_of_CPU_bursts_executed, time_spent_ready_list, count_io_device1, count_io_device2, startTime, finishTime, arrivalTime, turnoverTime, total_time_spent_CPU);
    enqueue(queue, PCB);
    if (OUTMODE == 1)
    {
    }
    else if (OUTMODE == 2)
    {
    }
    else if (OUTMODE == 3)
    {
        printf("Thread with pid %d added to queue\n", PCB->pid);
    }
    nextToQueue++;
    CPU(queue, PCB);
    // printf("thread sonu %d \n", PCB->pid);
    pthread_exit(NULL);
    // wait until turn
    // while(pid != count){condwait}
    // awake
    // sleep
    // dequeue // bitmediyse enqueue
}

void *control(void *controlsArgument)
{
    pthread_cond_t cv;
    pthread_cond_init(&(cv), NULL);
    while (1)
    {   
        if(allFinished==1){
            pthread_exit(3);
        }
        while (CPUBusy == 1)
        {
            ;
        }
        pthread_cond_broadcast(&(((struct arg2 *)controlsArgument)->que->cvReadyQueue));
    }

}

void *createProcessGeneratorThread(void *arg)
{
    // printf("Genereate %d \n", ALLP);
    struct Queue *readyQueue = createQueue(); // initialize ready queue
    pthread_t allThreads[1000];               // allocation for all threads
    pthread_t controller;
    struct arg2 controlsArgument;
    controlsArgument.que = readyQueue;
    int controllerThreadId = pthread_create(&(controller), NULL, control, (void *)&controlsArgument);

    for (int i = 0; i < ALLP; i++)
    {
        allThreads[i] = 0;
    }
    int countOfImmidiate = 0;
    struct arg point[1000];
    for (int i = 0; i < ALLP; i++)
    {
        point[i].pid = 0;
        point[i].que = 0;
        point[i].thread_id = 0;
    }
    if (MAXP < 10)
    {
        countOfImmidiate = MAXP;
    }
    else
    {
        countOfImmidiate = 10;
    }
    remainingProcessCount = ALLP - countOfImmidiate;
    // countofımm = allp - remrocess

    for (int i = 0; i < countOfImmidiate; i++)
    {
        point[i].pid = i;
        point[i].que = readyQueue;
        // printf("ben yaratilcam %d \n", point[i].pid);
        point[i].thread_id = pthread_create(&(allThreads[i]), NULL, addThreadToQueue, (void *)&point[i]);
        if (OUTMODE == 1)
        {
        }
        else if (OUTMODE == 2)
        {
        }
        else if (OUTMODE == 3)
        {
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            printf("Thread with id %d Arrived at: %02d,%02d\n", point[i].pid, tm.tm_min, tm.tm_sec);
        }
        ++processCount;
    }

    while (remainingProcessCount > 0)
    {
        // printf("allp kismi %d ", ALLP - remainingProcessCount);

        // int randomVar = ((double)rand() / (RAND_MAX));
        if (numberOfProcessesAtQueue <= MAXP && pg >= 0.5) // randomVar)
        {
            usleep(5000000);
            point[ALLP - remainingProcessCount].pid = ALLP - remainingProcessCount;
            point[ALLP - remainingProcessCount].que = readyQueue;
            point[ALLP - remainingProcessCount].thread_id = pthread_create(&(allThreads[ALLP - remainingProcessCount]), NULL, addThreadToQueue, (void *)&(point[ALLP - remainingProcessCount]));
            processCount++;
            if (OUTMODE == 1)
            {
            }
            else if (OUTMODE == 2)
            {
            }
            else if (OUTMODE == 3)
            {
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                printf("Thread with id %d Arrived at: %02d,%02d\n", point[ALLP - remainingProcessCount].pid, tm.tm_min, tm.tm_sec);
            }
            remainingProcessCount--;
        }
    }
   
    for (int i = 0; i < ALLP; i++)
    {
        pthread_join(allThreads[i], NULL);
    }
     allFinished=1;
}

void CPU(struct Queue *queue, struct PCB *pcb)
{
    if (algo == 0)
    {
        while (pcb->pid != queue->startNode->pid)
        { // en baştakini seçiyoruz sırayla
            // printf("bekliom benn = %d\n" , pcb->pid);
            // printf("start node %d \n ",queue->startNode->pid);
            // printf("start nodeun previ = %d \n",queue->startNode->prev->pid);
            pthread_cond_wait((&queue->cvReadyQueue), &(queue->lock));
        }
        // pthread_mutex_lock(&(queue->lock));
        long CPUTimeStart = gettimeofday(&start, NULL);
        // printf("calisiom benn = %d\n", pcb->pid);
        CPUBusy = 1;
        if (OUTMODE == 1)
        {
        }
        else if (OUTMODE == 2)
        {
        }
        else if (OUTMODE == 3)
        {
            printf("Thread with pid %d assigned to cpu\n", pcb->pid);
        }
        usleep(burstlen * 1000000);

        long CPUTimeEnd = gettimeofday(&end, NULL);
        if (OUTMODE == 1)
        {
        }
        else if (OUTMODE == 2)
        {
            printf("Thread with pid %d running\n", pcb->pid);
        }
        else if (OUTMODE == 3)
        {
            long runTime = end.tv_sec - start.tv_sec;
            printf("Thread with pid %d running\n", pcb->pid);
            printf("Run Time: %ld seconds\n", runTime);
        }
        dequeue(queue);
        if (OUTMODE == 1)
        {
        }
        else if (OUTMODE == 2)
        {
        }
        else if (OUTMODE == 3)
        {
            printf("Thread with pid %d finished\n", pcb->pid);
        }
        CPUBusy = 0;
        numberOfProcessesAtQueue--;
        pthread_mutex_unlock(&(queue->lock));
        // usleep(1000000);
        // pthread_cond_broadcast(&(queue->cvReadyQueue));
    }
    else if (algo == 1)
    {

        /* if(pcb->prev == NULL){
             printf("ben şuan bastrayim %d ", pcb->thread_id);
         }*/
        while (pcb->remaining_CPU_burst_length != 0)
        {
            while (pcb->pid != queue->startNode->pid)
            { // en baştakini seçiyoruz sırayla
                // printf("bekliom benn = %d\n" , pcb->pid);

                pthread_cond_wait((&queue->cvReadyQueue), &(queue->lock));
            }
            if (Q < pcb->remaining_CPU_burst_length)
            {
                long CPUTimeStart = gettimeofday(&start, NULL);
                CPUBusy = 1;
                // printf("calisiom benn = %d\n", pcb->pid);
                usleep(Q * 1000000);
                long CPUTimeEnd = gettimeofday(&end, NULL);
                if (OUTMODE == 1)
                {
                }
                else if (OUTMODE == 2)
                {
                    printf("Thread with pid %d running\n", pcb->pid);
                }
                else if (OUTMODE == 3)
                {
                    long runTime = end.tv_sec - start.tv_sec;
                    printf("Thread with pid %d running\n", pcb->pid);
                    printf("Run Time: %ld seconds\n", runTime);
                }
                dequeue1(queue);

                // printf("id check %d \n ", pcb->pid);
                pcb->remaining_CPU_burst_length = pcb->remaining_CPU_burst_length - Q;
                enqueue(queue, pcb); // PCB de var bu variable
                CPUBusy = 0;
            }
            else if (Q >= pcb->remaining_CPU_burst_length)
            {
                CPUBusy = 1;
                long CPUTimeStart = gettimeofday(&start, NULL);
                // printf("calisiom benn ve bitiriyorum = %d\n", pcb->pid);
                usleep(pcb->remaining_CPU_burst_length * 1000000); // BİTTİ REMAİNİNG TİME HESAPLAMAYA GEREK YOK
                long CPUTimeEnd = gettimeofday(&end, NULL);
                if (OUTMODE == 1)
                {
                }
                else if (OUTMODE == 2)
                {
                    printf("Thread with pid %d running\n", pcb->pid);
                }
                else if (OUTMODE == 3)
                {
                    long runTime = end.tv_sec - start.tv_sec;
                    printf("Thread with pid %d running\n", pcb->pid);
                    printf("Run Time: %ld seconds\n", runTime);
                }
                pcb->remaining_CPU_burst_length = 0;
                dequeue(queue);
                if (OUTMODE == 1)
                {
                }
                else if (OUTMODE == 2)
                {
                }
                else if (OUTMODE == 3)
                {
                    printf("Thread with pid %d finished\n", pcb->pid);
                }
                CPUBusy = 0;
                numberOfProcessesAtQueue--;
            }
            pthread_mutex_unlock(&(queue->lock));
        }
    }
    /*else if(algo == 2){//BU ALGORITMADAN TAM EMIN DEILIM BUNA GELECEZ

        int remainingTime = 0;
        while(pcb->next != NULL){

        }
        while(pid != pickshortestNextCpuBurst){//en kısa olmadığın sürece sen bi bekle
            pthread_cond_wait((&queue->cvReadyQueue), &(queue->lock));
        }
        int CPUTimeStart = gettimeofday();
        sleep(nextBurstLength);
        int CPUTimeEnd = gettimeofday();
        remainingCpuBurstLength = remainingCPuBurstLength - timeQuantum;
        if(remBUrstLength >= 0){
            removeFromQueue;
        }
        pthread_mutex_unlock(&(queue->lock));
    }*/
}

int main(int argc, char *argv[])
{
    // usleep(3);
    // printf("main");

    ALG = argv[1]; // algorithm

    if (strcmp(ALG, "RR") == 0)
    {
        algo = 1;
    }
    if (strcmp(ALG, "FCFS") == 0)
    {
        algo = 0;
    }
    if (strcmp(ALG, "SJF") == 0)
    {
        algo = 2;
    }

    Q = atoi(argv[2]); // time quantum
    if (strcmp(ALG, "RR") != 0)
    {
        Q = 0;
    }
    T1 = atoi(argv[3]); // service time of device 1
    T2 = atoi(argv[4]); // service time of device 2

    burst_dist = argv[5];      // fix,uniform or exponential
    burstlen = atoi(argv[6]);  // burst lenght
    min_burst = atoi(argv[7]); // min burst
    max_burst = atoi(argv[8]); // max burst
    p0 = atof(argv[9]);        // prob 1
    p1 = atof(argv[10]);       // prob 2
    p2 = atof(argv[11]);       // prob 3
    pg = atof(argv[12]);       // prob of creating new thread
    MAXP = atoi(argv[13]);     // max thread count 1<= MAXP <= 50
    ALLP = atoi(argv[14]);     // all thread count 1<= ALLP <= 1000
    OUTMODE = atoi(argv[15]);

    if (strcmp(burst_dist, "uniform") == 0)
    {
        burstlen = rand() % (max_burst + 1 - min_burst) + min_burst;
    }

    // printf("calisiom benn = %f\n", pg);
    // printf("ALG benn = %d\n", ALG);
    // printf("burst_dist benn = %d\n", burst_dist);
    /*burstlen = 14;
    MAXP = 3;
    ALLP = 3;
    min_burst=1;
    max_burst = 20;
    pg= 0.9;// choose output*/

    /*
    if(OUTMODE == 1){
        printf("");
    }
    else if(OUTMODE == 2){

    }
    else if(OUTMODE == 2){

    }
    */
    pthread_t mainthread;
    int thread_id = pthread_create(&mainthread, NULL, createProcessGeneratorThread, NULL);

    pthread_exit(mainthread);

    return 0;
}