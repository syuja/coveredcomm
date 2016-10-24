#
# Now, only the one file will be recompiled
#
CC=gcc
CFLAG=-pthread -g 
blink: blink.o 
	$(CC) $(CFLAG) blink.o -o blink
blink.o: blink.c 
	$(CC) -c blink.c
clean:
	rm -f a.out
	rm -f *.o
	rm -f *~
	rm -f core
	rm -f blink
