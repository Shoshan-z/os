CC=gcc
CFLAGS= -Wall -Werror -g

all: fifo_reader fifo_writer mmap_reader mmap_writer

fifo_reader: fifo_reader.c 
	$(CC) -o fifo_reader fifo_reader.c $(CFLAGS) 


fifo_writer: fifo_writer.c 
	$(CC) -o fifo_writer fifo_writer.c $(CFLAGS) 


mmap_reader: mmap_reader.c 
	$(CC) -o mmap_reader mmap_reader.c $(CFLAGS) 


mmap_writer: mmap_writer.c 
	$(CC) -o mmap_writer mmap_writer.c $(CFLAGS) 



.PHONY: clean

clean:
	rm -f fifo_reader fifo_writer mmap_reader mmap_writer 

