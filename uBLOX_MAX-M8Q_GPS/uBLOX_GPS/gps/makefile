gps: i2c.o ublox.o main.o trace.o
	cc -o gps i2c.o ublox.o main.o trace.o -lstdc++ -lm -lwiringPi -lwiringPiDev -lrt

main.o: main.cpp main.h
	gcc -c main.cpp

ublox.o: ublox.cpp ublox.h i2c.h
	gcc -c ublox.cpp

i2c.o : i2c.cpp i2c.h
	gcc -c i2c.cpp

trace.o : trace.cpp trace.h
	gcc -c trace.cpp
