.PHONY: clean
O=0
CFLAGS=-O${O} -g
targets=elfsymtool
ELFHDR=/usr/include/elf.h
DESCS_SRC=EMACHINE.c EVERSION.c ETYPE.c EICLASS.c EIDATA.c EIOSABI.c SHTYPE.c SHFLAGS.c STTYPE.c STBIND.c STVISIBILITY.c

DESCS_OBJECTS=${DESCS_SRC:.c=.o}

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

all: ${targets}

EMACHINE.c: ${ELFHDR}
	./mkdesc $@ EM_

EVERSION.c: ${ELFHDR}
	./mkdesc $@ EV_

ETYPE.c: ${ELFHDR}
	./mkdesc $@ ET_

EICLASS.c: ${ELFHDR}
	./mkdesc $@ ELFCLASS

EIDATA.c: ${ELFHDR}
	./mkdesc $@ ELFDATA

EIOSABI.c: ${ELFHDR}
	./mkdesc $@ ELFOSABI

SHTYPE.c: ${ELFHDR}
	./mkdesc $@ SHT_

SHFLAGS.c: ${ELFHDR}
	./mkdesc $@ SHF_

STTYPE.c: ${ELFHDR}
	./mkdesc $@ STT_

STBIND.c: ${ELFHDR}
	./mkdesc $@ STB_

STVISIBILITY.c: ${ELFHDR}
	./mkdesc $@ STV_

ELF.o: ELF.c ELF.h

elfsymtool: ELF.o ${DESCS_OBJECTS} elfsymtool.o 
	${CC} ${LDFLAGS} -o $@ $^


clean:
	${RM} -rf $(wildcard ${targets} ${DESCS_SRC} *.o)



