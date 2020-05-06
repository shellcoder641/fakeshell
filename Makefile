all: fshell UDP_listener
CC=gcc
CFLAGS=-g 
LIBS= -lreadline -lssl -lcrypto
fshell: main.c
	$(CC) -O3 main.c -o fshell $(LIBS)
UDP_listener: UDP_listener.c
	$(CC) -O3 UDP_listener.c -o UDP_listener -lssl -lcrypto
clean:
	rm fshell
	rm UDP_listener
