#ifndef __ELF_H__
#define __ELF_H__

#include "desc.h"
#include "elfdescs.h"

static void 
printDesc(struct Desc *dtbl, int v, char *name, FILE *fp)
{
  for (int i=0; dtbl[i].val != -1; i++) {
    if (dtbl[i].val == v) {
      fprintf(fp, "%s:%d\t- %s : %s\n", name,
	      dtbl[i].val, dtbl[i].name, dtbl[i].desc);
      return;
    }
  }
  assert(0);
}

union EIDENT {
  uint8_t raw[EI_NIDENT];
  struct {
    uint8_t m0;
    uint8_t m1;
    uint8_t m2;
    uint8_t m3;
    uint8_t class;
    uint8_t data;
    uint8_t version;
    uint8_t osabi;
    uint8_t abiversion;
  } values;
};

typedef union {
  Elf64_Addr a64;
  Elf32_Addr a32;
} ElfN_Addr;

typedef union {
  Elf64_Off o64;
  Elf32_Off o32;
} ElfN_Off;

typedef union {
  Elf64_Ehdr s64;
  Elf32_Ehdr s32;
} ElfN_Ehdr;

typedef union {
  Elf64_Shdr s64;
  Elf32_Shdr s32;
} ElfN_Shdr;


typedef union {
  Elf64_Sym s64;
  Elf32_Sym s32;
} ElfN_Sym;

// globally tracks the class of elf file we are working with
// we expect it to be either ELFCLASS64 or ELFCLASS32
// check once that is is either of these values and then all code
// assumes so
extern int ElfN;

static void setElfN(int class) {
  assert(class == ELFCLASS64 || class == ELFCLASS32);
  ElfN=class;
}

// set or get a field of a ElfN structure via ElfN union
#define NSSET(s,f,v) ((ElfN == ELFCLASS64) ? s.s64.f = v : s.s32.f = v)
#define NSGET(s,f) ((ElfN == ELFCLASS64) ? s.s64.f : s.s32.f)

#define NA(p,f) ((ElfN == ELFCLASS64) ? &(p->s64.f) : &(p->s32.f))
// set or get a field of a ElfN structure via ElfN union pointer
#define NPSET(p,f,v) ((ElfN == ELFCLASS64) ? p->s64.f = v : p->s32.f = v)
#define NPGET(p,f) ((ElfN == ELFCLASS64) ? p->s64.f : p->s32.f)

// the following allows us to work with arrays of ElfN unions as if they
// were an array of the specific ElfN class struct
// results in a pointer to the ith element from base address a
#define  NI(a,i) ((ElfN == ELFCLASS64) ? ((void *)a) + (i*sizeof(a[0].s64)) \
		  : ((void *)a) + (i*sizeof(a[0].s32)));

#define MAX_PATH 80

enum ELFSTATE { NONE=0, OPEN=1<<0, MAPPED=1<<1, STRSFOUND=1<<2, SYMSFOUND=1<<3, DIRTY=1<<4 };
typedef struct {
  char path[MAX_PATH];
  struct stat finfo;
  uint8_t *addr;
  int fd;
  int openflgs;
  int mmapflgs;
  int protflgs;
  enum ELFSTATE state;
  int valid;
  int class;
  int dataencoding;
} ELF;

static inline int ELFvalid(ELF *elf) { return elf->valid; }
static inline int ELFclass(ELF *elf)     { return elf->class; }
static inline int ELFdataencoding(ELF *elf) {
  return elf->dataencoding;
}
int ELFopen(ELF *elf, char *path, int openflgs, int persist);

static void fprintElfN_Addr(FILE *fp, ElfN_Addr addr) {
  if (ElfN == ELFCLASS64) fprintf(fp, "0x%" PRIx64, addr.a64);
  else fprintf(fp, "0x%" PRIx32, addr.a32);
}

static void fprintElfN_Off(FILE *fp, ElfN_Off off) {
  if (ElfN == ELFCLASS64) fprintf(fp, "%" PRId64, off.o64);
  else fprintf(fp, "0x%" PRId32, off.o32);
}

void ELFprintHdr(ELF *elf, FILE *fp);
void ELFprintSections(ELF *elf, FILE *fp);

#endif
