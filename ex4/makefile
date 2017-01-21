CC=gcc
CFLAGS= -Wall -Werror -g

all: os_client os_server

os_client: os_client.c 
	$(CC) -o os_client os_client.c $(CFLAGS) 


os_server: os_server.c 
	$(CC) -o os_server os_server.c $(CFLAGS) 

.PHONY: clean

clean:
	rm -f os_client os_server 

