.PHONY: clean
O=0
CFLAGS=-O${O} -g
targets=elfsymtool

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

elfsymtool: elfsymtool.o
	${CC} ${LDFLAGS} -o $@ $^


all: ${targets}



