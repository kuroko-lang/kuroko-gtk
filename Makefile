all: gtk.so

CFLAGS=-I../kuroko/src/ $(shell pkg-config --cflags gtk+-3.0) -fPIC
LDFLAGS=-L../kuroko/
LDLIBS=-lkuroko $(shell pkg-config --libs gtk+-3.0)
gtk.so: gtk.c
	${CC} ${CFLAGS} ${LDFLAGS} -shared -o $@ $< ${LDLIBS}

.PHONY: clean
clean:
	-rm gtk.so
