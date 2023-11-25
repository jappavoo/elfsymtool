#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>
#include "ELF.h"

int ElfN = ELFCLASSNONE;

// used only during open after that these values
// are cached in elf object so use methods to get them
static int getELFClass(ELF *elf) {
  Elf64_Ehdr *hdr = (void *)(elf->addr);
  union EIDENT *ident = (void *)&(hdr->e_ident);
  return ident->values.class;
}

static int getELFDataEncoding(ELF *elf) {
  Elf64_Ehdr *hdr = (void *)(elf->addr);
  union EIDENT *ident = (void *)&(hdr->e_ident);
  return ident->values.data;
}

static bool validElfIdent(ELF *elf) {
  Elf64_Ehdr *hdr = (void *)(elf->addr);
  union EIDENT *ident = (void *)&(hdr->e_ident);
  return (ident->values.m0 == ELFMAG0 &&
	  ident->values.m1 == ELFMAG1 &&
	  ident->values.m2 == ELFMAG2 &&
	  ident->values.m3 == ELFMAG3 );
}

int
ELFopen(ELF *elf, char *path, int openflgs, int persist)
{
  struct stat statbuf;
  int fd;
  int mmapflgs, protflgs;
  void *bytes;
  enum ELFSTATE state = NONE;
  int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  assert(elf);
  assert(path);

  fd = open(path, openflgs, mode);
  if (fd == -1) {
    warn(__FUNCTION__);
    return 0;
  }
  state |= OPEN;
  
  if (fstat(fd, &statbuf) == -1) {
    warn(__FUNCTION__);
    return 0;    
  }

  // If we are presisting the changes then we map shared
  // otherwise we map private
  mmapflgs =  (persist) ?  MAP_SHARED : MAP_PRIVATE; // flags
  if (openflgs & O_RDONLY)  protflgs = PROT_READ;
  else protflgs = PROT_READ | PROT_WRITE;
  bytes = mmap(NULL, // addr
	       statbuf.st_size, // length,
	       protflgs, // prot
	       mmapflgs,
	       fd, // fd,
	       0);  // offset
  if (bytes == MAP_FAILED) {
    warn(__FUNCTION__);
    return 0;
  }
  state |= MAPPED;
  
  strncpy(elf->path, path, sizeof(elf->path));
  elf->openflgs = openflgs;
  elf->fd = fd;
  memcpy(&(elf->finfo), &statbuf, sizeof(statbuf));
  elf->mmapflgs = mmapflgs;
  elf->protflgs = protflgs;
  elf->addr = bytes;
  elf->state = state;
  elf->valid = validElfIdent(elf);
  if (!ELFvalid(elf)) {
    fprintf(stderr, "ERROR: does not seem to be an elf file bad ident\n");
    return 0;
  }
  elf->class = getELFClass(elf);
  elf->dataencoding = getELFDataEncoding(elf);
  setElfN(elf->class);
  // a couple of sanity check as I doubt the code
  // is robust to anything other than these
  assert(ELFclass(elf) == ELFCLASS64 ||
	 ELFclass(elf) == ELFCLASS32);
  assert(ELFdataencoding(elf) == ELFDATA2LSB);
  return 1;
}

static void 
fprintIdent(FILE *fp, union EIDENT *ident) {
  fprintf(fp, "ident:\n");
  fprintf(fp, "\tmagic[0]:0x%02hhx magic[1]:0x%02hhx"
	  " magic[2]:0x%02hhx magic[3]:0x%02hhx\n",
	  ident->values.m0, ident->values.m1,
	  ident->values.m2, ident->values.m3);
  printDesc(EICLASS, ident->values.class, "\tclass", fp);
  printDesc(EIDATA, ident->values.data, "\tdata", fp);
  printDesc(EVERSION, ident->values.version, "\tver", fp);
  printDesc(EIOSABI, ident->values.osabi, "\tosabi", fp);
  fprintf(fp, "\tabiver:%d\n", ident->values.abiversion);
}

void ELFprintHdr(ELF *elf, FILE *fp)
{
  ElfN_Ehdr *hdr = (void *)elf->addr;

  fprintf(fp, "Header:\n");
  fprintIdent(fp, (union EIDENT *)NA(hdr,e_ident));
  printDesc(ETYPE, NPGET(hdr,e_type), "e_type", fp);
  printDesc(EMACHINE, NPGET(hdr,e_machine), "e_machine", fp);
  printDesc(EVERSION, NPGET(hdr,e_version), "e_version", fp);
  fprintf(fp, "en_entry:");
  fprintElfN_Addr(fp, (ElfN_Addr)(NPGET(hdr,e_entry)));
  fprintf(fp, "\t- virtual address of entry entry point\n");
  //  ELFprintAddr(elf, "e_entry", (ElfN_Addr)hdr->e_entry,
  //	    "\t- , fp);
  fprintf(fp, "e_poff:");
  fprintElfN_Off(fp, (ElfN_Off)(NPGET(hdr,e_phoff)));
  fprintf(fp, "\t- program header table's file offset in bytes\n");
  fprintf(fp, "e_shoff:");
  fprintElfN_Off(fp, (ElfN_Off)NPGET(hdr,e_shoff));
  fprintf(fp, "\t- section header table's file offset in bytes\n");
  fprintf(fp, "e_flags:0x%" PRIx32
	  "\t- currently no processor-specific flags have been define\n",
	  NPGET(hdr,e_flags));
  fprintf(fp, "e_ehsize:%" PRIu16
	  "\t- ELF header's size in bytes\n", NPGET(hdr,e_ehsize));
  fprintf(fp, "e_phentsize:%" PRIu16
	  "\t- size in bytes of one entry in the "
	  "program header table\n", NPGET(hdr,e_phentsize));
  fprintf(fp, "e_phnum:%" PRIu16
	  "\t- number ofentries in the program header table\n",
	  NPGET(hdr,e_phnum));
  fprintf(fp, "e_shentsize:%" PRIu16
	  "\t- size in bytes of one entry in the"
	  " section header table \n", NPGET(hdr,e_shentsize));
  fprintf(fp, "e_shnum:%" PRIu16
	  "\t- number of entries in the section header table\n",
	  NPGET(hdr,e_shnum));
  fprintf(fp, "e_shstrndx:%" PRIu16
	  "\t- section header table index of the entry\n\t\t  "
	  "associated with the section name string table\n",
	  NPGET(hdr,e_shstrndx));
}


char *
ELFString(ELF *elf, int i) {
  return "";
}

void ELFprintSym(ELF *elf, ElfN_Sym *sym, FILE *fp)
{
#if 0
  switch (ELFclass(elf)) {
  case ELFCLASS32:
    fprintf(fp, "st_name:%" PRId32 "(%s)", sym->s32.st_name,
	    ELFString(elf, sym->s32.st_name));
    ELFprintAddr(elf, "st_value", (ElfN_Addr)sym->s32.st_value, "", fp);
    break;
  case ELFCLASS64:
    fprintf(fp, "st_name:%" PRId32 "(%s)", sym->s64.st_name,
	    ELFString(elf, sym->s64.st_name));
    ELFprintAddr(elf, "st_value", (ElfN_Addr)sym->s64.st_value, "", fp);
    break;
  default: assert(0);
  }
#endif
}

void ELFprintSections(ELF *elf, FILE *fp)
{
  ElfN_Ehdr *hdr = (void *)elf->addr;
  ElfN_Shdr *shdrs = (void *)(elf->addr + NPGET(hdr,e_shoff));
  int n = NPGET(hdr,e_shnum);

  fprintf(fp, "Section Header Table:\n");
  for (int i=0; i<n; i++) {
    ElfN_Shdr *sh = NI(shdrs,i);
    fprintf(stderr, "section[%d]:",i);
    printDesc(SHTYPE,
	      NPGET(sh,sh_type), "sh_type", fp);
    
  }
  
}

