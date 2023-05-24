all: histserver histclient histclient_th histserver_th

histserver: histserver.c
	gcc -Wall -o server histserver.c -lm -lrt
	
histclient: histclient.c
	gcc -Wall -o client histclient.c -lm -lrt

histserver_th: histserver_th.c
	gcc -Wall -o server_th histserver_th.c -lm -lrt -lpthread
	
histclient_th: histclient_th.c
	gcc -Wall -o client_th histclient_th.c -lm -lrt -lpthread
	
clean:
	rm -fr *~ histserver histclient