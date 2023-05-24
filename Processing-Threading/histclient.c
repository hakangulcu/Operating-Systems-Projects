 
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "lastMessage.h"           
#include "shareddefs.h"
#include <string.h>
 
int main(int argc, char **argv)
{

    const int intervalStart = (atoi)(argv[3]);
	const int intervalWidth = (atoi)(argv[2]);
    const int intervalCount = (atoi)(argv[1]);

	mqd_t mq;
	struct item item;
	int n;
	char *bufptr3;
    long buflen3;
	struct mq_attr mq3_attr;
	struct item3 *itemptr3;
	struct item3 item3;

	mq = mq_open(MQNAME, O_RDWR);
	if (mq == -1) {
		perror("can not open msg queue\n");
		exit(1);
	}
	printf("mq opened, mq id = %d\n", (int) mq);
	int i = 0;

	
		item.id = i;
        item.intervalCount = intervalCount;
        item.intervalWidth = intervalWidth;
        item.intervalStart = intervalStart;

		strcpy(item.astr, "this string is sample data\n");

		n = mq_send(mq, (char *) &item, sizeof(struct item), 0);

		if (n == -1) {
			perror("mq_send failed\n");
			exit(1);
		}

		printf("mq_send success, item size = %d\n",
		       (int) sizeof(struct item));
		printf("item->id   = %d\n", item.id);
		printf("item->astr = %s\n", item.astr);
		printf("\n");
        printf("item->count   = %d\n", item.intervalCount);
        printf("item->width   = %d\n", item.intervalWidth);
        printf("item->start  = %d\n", item.intervalStart);
		i++;
		sleep(2);
	

	mq_close(mq);

	mqd_t mq3;
	mq3 = mq_open(MQNAME3, O_RDWR);
	/* allocate large enough space for the buffer to store
        an incoming message */

	mq_getattr(mq3, &mq3_attr);
    printf("mq3 maximum msgsize = %d\n", (int)mq3_attr.mq_msgsize);


    buflen3 = mq3_attr.mq_msgsize;
    bufptr3 = (char *)malloc(buflen3);
	itemptr3 = (struct item3 *)bufptr3;

	if (mq == -1) {
		perror("can not open msg queue\n");
		exit(1);
	}
	printf("mq3 opened, mq3 id = %d\n", (int) mq3);
	
	int mess = mq_receive(mq3, (char *)bufptr3, buflen3, NULL);
 
        if (mess == -1)
        {
            perror("mq_receive failed\n");
            exit(1);
        }

        printf("mq_receive success, message size = %d\n", n);

	for(int i = 0; i < item.intervalCount; i++){
		printf("Result: %d\n", itemptr3->lastFrequency[i]);
	}

	
	int v1 = intervalStart;
    int v2 = intervalWidth;
    int v3 = v1+v2;
	for(int i = 0; i < item.intervalCount; i++){
        printf("\n[%d,%d):%d",v1,v3,itemptr3->lastFrequency[i]);
        v1 = v3;
        v3 = v3 + v2;
	}
	messageW *m = ((messageW * )malloc(sizeof(messageW)));
	strcpy(m->lastMessage, "See you");
	strcpy(itemptr3->lastMessage, m->lastMessage);
	item3.finish = 1;
	int finalCLientMessage = mq_send(mq3, (char *) &item3, sizeof(struct item3), 0);
	printf("Final message from Client: %s\n", itemptr3->lastMessage);
		if (finalCLientMessage == -1) {
			perror("mq_send failed\n");
			exit(1);
		}

		printf("mq3_send success, item size = %d\n",
		       (int) sizeof(struct item3));
	mq_close(mq);
    mq_close(mq3);
	printf("All message queues closed\n");
	return 0;
}