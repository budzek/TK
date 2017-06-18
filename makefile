all: comp
CC=g++ -std=c++11
CFLAGS=-g
FLEXPARM=-lfl
OBJS=emitter.o lexer.o main.o parser.o symbol.o
 

global.h: parser.h

lexer.c: lexer.l global.h
	flex -o $@ $<

parser.c parser.h: parser.y
#	bison -o parser.c -d parser.y
	bison -v -o parser.c -d parser.y


lexer.o: lexer.c global.h
	$(CC) $(CFLAGS) -c $< $(FLEXPARM)

#$< pierwszy składnik $@cel
%.o: %.c global.h
	$(CC) $(CFLAGS) -c $< -o $@ 
#$^ wszystkie składniki
comp: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(FLEXPARM)
	echo "Program skompilowany poprawnie"
clean:
	rm -f *.o
	rm -f comp
	rm -f lexer.c
	rm -f parser.c
	rm -f parser.h
.PHONY: all clean
