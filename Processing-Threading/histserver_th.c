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

struct arg {

    int intervalEnd, intStart, intWidth;
    int intCount;
    mqd_t mq2;
    FILE *file;
};
static void *do_task(void *arg_ptr)
{
	
    //struct arg *s;
    int no = 0;
    int ret = 0;
    int intervalFrequency[((struct arg *) arg_ptr)->intCount];

    for (int j = 0; j < ((struct arg *) arg_ptr)->intCount; j++)
    {
        intervalFrequency[j] = 0;
    }
            int k = 0;
            fscanf(((struct arg *) arg_ptr)->file, "%d", &k);
            while (!feof(((struct arg *) arg_ptr)->file))
            { 
                fscanf(((struct arg *) arg_ptr)->file, "%d", &k);

                if ((k < ((struct arg *) arg_ptr)->intervalEnd) && (k >= ((struct arg *) arg_ptr)->intStart))
                {
                    if ((k - ((struct arg *) arg_ptr)->intStart) % ((struct arg *) arg_ptr)->intWidth == 0)
                    {
                        no = (k - ((struct arg *) arg_ptr)->intStart) / ((struct arg *) arg_ptr)->intWidth + 1;
                    }
                    else
                    {
                        no = ceil((double)((double)((int)k - ((struct arg *) arg_ptr)->intStart) / ((struct arg *) arg_ptr)->intWidth));
                    }
                    intervalFrequency[no - 1]++;
                }
            }
            fclose(((struct arg *) arg_ptr)->file);
            
            struct item2 item2;
            for (int i = 0; i < ((struct arg *) arg_ptr)->intCount; i++)
            {

                item2.frequency[i] = intervalFrequency[i];
                intervalFrequency[i] = 0;
            }
            

            
            int abc = mq_send(((struct arg *) arg_ptr)->mq2, (char *)&item2, sizeof(struct item2), 0);
            //printf("asjdd%d", abc);
            if (abc == -1)
            {
                perror("mq_send failed\n");
                exit(1);
            }

            printf("mq_send success, item size = %d\n",
                   (int)sizeof(struct item2));
            
            pthread_exit(3);
}

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
    //pid_t p_id = 0;
    mqd_t mq2;
    int ret = 0;
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

    //int no;
    itemptr = (struct item *)bufptr;
    int intCount = itemptr->intervalCount;
    int intStart = itemptr->intervalStart;
    int intWidth = itemptr->intervalWidth;
    int finalFrequency[itemptr->intervalCount];

    for (int k = 0; k < intCount; k++)
    {
        finalFrequency[k] = 0;
    }
    //int intervalFrequency[intCount];
    //;
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

    

    int intervalEnd = intStart + ((intCount) * (intWidth));

    struct arg point[10];
    
    pthread_t threads[intCount];

    clock_t start_time = clock();
    for (int i = 1; i <= numberOfFiles; i++)
    {
        point[i-1].intCount = intCount;
        point[i-1].intervalEnd = intervalEnd;
        point[i-1].intStart = intStart;
        point[i-1].intWidth = intWidth;
        point[i-1].mq2 = mq2;
        FILE *file = fopen(argv[i + 1], "r");
        point[i-1].file = file;
        ret = pthread_create(&(threads[i-1]),
				     NULL, do_task, (void *) &(point[i-1]));

		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		printf("thread %i with tid %u created\n", i,
		       (unsigned int) threads[i-1]);
    }
    clock_t end_time = clock();

    for (int j = 0; j < numberOfFiles; j++)
    {
        
        int z = mq_receive(mq2, (char *)bufptr2, buflen2, NULL);
        pthread_join(threads[j], NULL);
        
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

    /*int st = waitpid(-1, NULL, WNOHANG);
    if(st == -1){
        printf("All childs threads ended.\n");
    }*/
        
    printf("All message queues closed\n");
    
    printf("Running time for threads: %zd milliseconds.\n", end_time - start_time);
    
    return 0;
}
