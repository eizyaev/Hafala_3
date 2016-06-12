# Makefile for the tftp program
CC = gcc
CFLAGS = -std=c99 -g -Wall
CCLINK = $(CC)
OBJS = server.o 
RM = rm -f
# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) $(CFLAGS) -o ttftps $(OBJS)
# Creating the object files
server.o: server.c
	$(CC) $(CFLAGS) -c server.c
# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* cacheSim*

