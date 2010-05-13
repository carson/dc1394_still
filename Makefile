CC = gcc
CFLAGS = -Wall -Wunused -std=gnu99 -g -O0 

dc1394_still: dc1394_still.o
	$(CC) -o $@ dc1394_still.o -ldc1394

dc1394_still.o: dc1394_still.c
	$(CC) -c $<
clean:
	rm dc1394_still *.o
