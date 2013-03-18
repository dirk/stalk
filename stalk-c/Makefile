CC=clang
CFLAGS=-g -Wall -c -std=c99 -DHASH_FUNCTION=HASH_FNV
LDFLAGS=-lfl

LEX=flex
LFLAGS=


# PARSE=bison
PARSE=/usr/local/Cellar/bison/2.7/bin/bison

# SOURCES=types.c system.c stalk.c
SOURCES=symbol.c data.c syntax.c stalk.yy.c parse.tab.c bootstrap.c stalk.c
EXECUTABLE=stalk
EXDIR=bin
OBJECTS=$(SOURCES:.c=.o)


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	chmod +x $(EXECUTABLE)
	mkdir -p $(EXDIR)
	cp $(EXECUTABLE) $(EXDIR)/$(EXECUTABLE)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Override default lex and yacc rule
%.c: %.y
%.c: %.l

parse.tab.c: parse.y
	$(PARSE) -d -o $@ $^

stalk.yy.c: stalk.l
	$(LEX) $(LFLAGS) -o $@ $^



clean:
	rm -rf *.o $(EXECUTABLE) $(EXDIR)/$(EXECUTABLE)

graph:
	$(PARSE) --graph=parse.dot -d -o parse.tab.c parse.y
