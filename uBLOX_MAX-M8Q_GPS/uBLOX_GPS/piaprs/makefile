piaprs: aprs.o json.o
	gcc -o piaprs aprs.o json.o

aprs.o: aprs.c aprs.h json.h
	gcc -c aprs.c

json.o: json.c json.h
	gcc -c json.c
