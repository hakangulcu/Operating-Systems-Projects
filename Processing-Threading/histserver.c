#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "message.h"
#include "lastMessage.h"
#include "shareddefs.h"
#include <time.h>


int main(int argc, char **argv)
{

    int p = 0;
    for (p = 0; p < argc; p++) {
        printf("argv[%d] = %s\n", p, argv[p]);
    }

    

    // hist details
    int numberOfFiles = (atoi)(argv[1]);
    printf("Number of files: %d\n", numberOfFiles);
    mqd_t mq;
    struct mq_attr mq_attr;
    struct item *itemptr;
    int n;
    char *bufptr;
    int buflen;
    char *bufptr2;
    long buflen2;
    char *bufptr3;
    long buflen3;
    struct mq_attr mq_attr2;
    struct item2 *itemptr2;
    struct item3 *itemptr3;
    struct mq_attr mq3_attr;
    struct item3 item3;
    mqd_t mq3;
    pid_t p_id = 0;
    mqd_t mq2;

    mq = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
    if (mq == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
    printf("mq created, mq id = %d\n", (int)mq);

    mq_getattr(mq, &mq_attr);
    printf("mq maximum msgsize = %d\n", (int)mq_attr.mq_msgsize);


    /* allocate large enough space for the buffer to store
        an incoming message */
    buflen = mq_attr.mq_msgsize;
    bufptr = (char *)malloc(buflen);

    n = mq_receive(mq, (char *)bufptr, buflen, NULL);
    if (n == -1)
    {
        perror("mq_receive failed\n");
        exit(1);
    }

    printf("mq_receive success, message size = %d\n", n);

    int no;
    itemptr = (struct item *)bufptr;
    int intCount = itemptr->intervalCount;
    int intStart = itemptr->intervalStart;
    int intWidth = itemptr->intervalWidth;
    int finalFrequency[itemptr->intervalCount];

    for (int k = 0; k < intCount; k++)
    {
        finalFrequency[k] = 0;
    }
    int intervalFrequency[intCount];

    ;
    mq2 = mq_open(MQNAME2, O_RDWR | O_CREAT, 0666, NULL);
    if (mq2 == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
    printf("mq created, mq id = %d\n", (int)mq2);
    mq_getattr(mq2, &mq_attr2);
    buflen2 = mq_attr2.mq_msgsize;
    bufptr2 = (char *)malloc(buflen);

    for (int j = 0; j < intCount; j++)
    {
        intervalFrequency[j] = 0;
    }
    int intervalEnd = intStart + ((intCount) * (intWidth));
    clock_t start_time = clock();
    for (int i = 1; i <= numberOfFiles; i++)
    {
        p_id = fork();

        if (p_id < 0)
        {
            fprintf(stderr, "fork failed");
        }
        else if (p_id == 0)
        {
            FILE *file = fopen(argv[i+1], "r");

            int k = 0;
            fscanf(file, "%d", &k);
            while (!feof(file))
            { // fgets(line, sizeof(line), file)){

                fscanf(file, "%d", &k);

                if ((k < intervalEnd) && (k >= intStart))
                {
                    if ((k - intStart) % intWidth == 0)
                    {
                        no = (k - intStart) / intWidth + 1;
                    }
                    else
                    {
                        no = ceil((double)((double)((int)k - intStart) / intWidth));
                    }
                    intervalFrequency[no - 1]++;
                }
            }
            fclose(file);
            struct item2 item2;
            for (int i = 0; i < intCount; i++)
            {
                item2.frequency[i] = intervalFrequency[i];
                intervalFrequency[i] = 0;
            }
            // mq_getattr(mq, &mq_attr2);
            // char *bufptr2;
            // int buflen2;
            // buflen2 = mq_attr2.mq_msgsize;
            // bufptr2 = (char *)malloc(buflen2);

            int abc = mq_send(mq2, (char *)&item2, sizeof(struct item2), 0);
            if (abc == -1)
            {
                perror("mq_send failed\n");
                exit(1);
            }

            printf("mq_send success, item size = %d\n",
                   (int)sizeof(struct item2));
            // fflush(NULL);
            // free(bufptr2);
            _exit(0);
            // sleep(60);
            // sleep(1);
            // signal(SIGALRM, killchild);
            // kill(getpid(), SIGCHLD);
        }
    }
    clock_t end_time = clock();
    for (int j = 0; j < numberOfFiles; j++)
    {
        int z = mq_receive(mq2, (char *)bufptr2, buflen2, NULL);
 
        wait(NULL);
        if (z == -1)
        {
            perror("mq_receive failed\n");
            exit(1);
        }

        printf("mq_receive success, message size = %d\n", n);

        itemptr2 = (struct item2 *)bufptr2;
        for (int i = 0; i < intCount; i++)
        {
            finalFrequency[i] = finalFrequency[i] + itemptr2->frequency[i];
        }
    }
    /*
    for (int i = 0; i < intCount; i++)
    {
            printf("* %d\n", finalFrequency[i]);
    }
    */
    // for (int j = 0; j < intCount; j++)
    //{
    //   printf("%d \n", intervalFrequency[j]);
    //}
    // int lastIntervalfrequency[20];
    /*for(int j = 0; j<5; j++){
      wait(process(j));
        n = mq_receive(mq, (char *) bufptr, buflen, NULL);
        for(intervalfrrquencyboyutu){
            lastintervalFrequncy[i] =  lastintervalFrequncy[i] + messag[i];
        }

    }*/

    printf("received item->id = %d\n", itemptr->id);
    printf("received item->astr = %s\n", itemptr->astr);
    printf("received item->count = %d\n", intCount);
    printf("received item->end = %d\n", intervalEnd);
    printf("received item->start = %d\n", intStart);
    printf("\n");

    free(bufptr);
    free(bufptr2);
    mq_close(mq);

    mq3 = mq_open(MQNAME3, O_RDWR | O_CREAT, 0666, NULL);
    if (mq3 == -1)
    {
        perror("can not create msg queue\n");
        exit(1);
    }
    printf("mq3 created, mq3 id = %d\n", (int)mq3);

    mq_getattr(mq3, &mq3_attr);
    printf("mq3 maximum msgsize = %d\n", (int)mq3_attr.mq_msgsize);
    // mq_close(mq);

    /* allocate large enough space for the buffer to store
        an incoming message */
    buflen3 = mq3_attr.mq_msgsize;
    bufptr3 = (char *)malloc(buflen3);
    itemptr3 = (struct item3 *)bufptr3;
    
    for(int u = 0; u < intCount; u++){
        item3.lastFrequency[u] = finalFrequency[u];
    }
    int lastSend = mq_send(mq3, (char *)&item3, sizeof(struct item3), 0);
            if (lastSend == -1)
            {
                perror("mq_send failed\n");
                exit(1);
            }

            printf("mq_send success, item size = %d\n",
                   (int)sizeof(struct item3));

    sleep(5);

    int mess2 = mq_receive(mq3, (char *)bufptr3, buflen3, NULL);
 
        if (mess2 == -1)
        {
            perror("mq_receive failed\n");
            exit(1);
        }

        printf("mq_receive success, message size = %d\n", n);
        
    printf("Final message from Client: %s\n", itemptr3->lastMessage);
    mq_close(mq);
    mq_close(mq2);
    mq_close(mq3);

    int st = waitpid(-1, NULL, WNOHANG);
    if(st == -1){
        printf("All childs processess ended.\n");
    }
        
    printf("All message queues closed\n");
    printf("Running time for processes: %zd milliseconds.\n", end_time - start_time);
    
    
    return 0;
}
