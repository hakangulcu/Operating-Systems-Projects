all: libdma.a  app test experiment experiment_2

libdma.a:  dma.c
	gcc -Wall -g -c -lpthread -lm -lrt dma.c 
	ar rcs libdma.a dma.o

app: app.c
	gcc -Wall -g -o app app.c -L. -ldma -lm -lpthread -lrt

test: test.c
	gcc -Wall -g -o test test.c -L. -ldma -lm -lpthread -lrt

experiment: experiment.c
	gcc -Wall -g -o experiment experiment.c -L. -ldma -lm -lpthread -lrt

experiment_2: experiment_2.c
	gcc -Wall -g -o experiment_2 experiment_2.c -L. -ldma -lm -lpthread -lrt

clean: 
	rm -fr *.o *.a *~ a.out  app test experiment experiment_2
