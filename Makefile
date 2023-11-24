.PHONY: clean
O=0
CFLAGS=-O${O} -g
targets=elfsymtool

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@


all: ${targets}

ELF.o: ELF.c ELF.h

elfsymtool: ELF.o elfsymtool.o 
	${CC} ${LDFLAGS} -o $@ $^


clean:
	${RM} -rf $(wildcard ${targets} *.o)



