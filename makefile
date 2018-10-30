OBJS = integer.o cda.o queue.o scanner.o
OPTS = -Wall -Wextra

hostd: dispatcher.c sigtrap.c $(OBJS)
	gcc -g dispatcher.c -o dispatcher -Wall $(OBJS)
	gcc -g sigtrap.c -o process -Wall $(OBJS)

scanner.o: scanner.c scanner.h
	gcc $(OPTS) -c scanner.c

integer.o: integer.c integer.h
	gcc $(OPTS) -c integer.c

cda.o: cda.c cda.h
	gcc $(OPTS) -c cda.c

queue.o: queue.c queue.h
	gcc $(OPTS) -c queue.c

clean:
	rm *.o dispatcher process