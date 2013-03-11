CC=gcc
CFLAGS=-g -Wall -c -std=c99 -DHASH_FUNCTION=HASH_FNV
LDFLAGS=

# SOURCES=types.c system.c stalk.c
SOURCES=symbol.c syntax.c data.c stalk.c
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


clean:
	rm -rf *.o $(EXECUTABLE) $(EXDIR)/$(EXECUTABLE)
