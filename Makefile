CC=gcc
CFLAGS=-g 
LIBS= -lreadline
qshell: main.c
	$(CC) main.c -o qshell $(LIBS)

clean:
	rm qshell
