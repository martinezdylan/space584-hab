OBJECTS=  filter.o  wfaltxt.o tunetones.o

CFLAGS=   -Wall -pedantic -O3  -I/usr/local/include -DFTTW_ENABLE_FLOAT -L/usr/local/lib -L/usr/X11/lib 

CC= gcc

all:	wfaltxt tunetones


wfaltxt:	filter.o wfaltxt.o
	$(CC) -o wfaltxt filter.o wfaltxt.o $(CFLAGS)    -lasound  -lfftw3   -lm 

tunetones:	filter.o tunetones.o
	$(CC) -o tunetones filter.o tunetones.o $(CFLAGS) -lasound -lfftw3 -lm

wfaltxt.o:	wfaltxt.c
	$(CC) -c wfaltxt.c 


filter.o:	filter.c
	$(CC) -c $(CFLAGS) filter.c

tunetones.o:	tunetones.c
	$(CC) -c tunetones.c

clean:	
	rm wfaltxt
	rm *.o

