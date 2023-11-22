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

struct Args {
  char *elfpath;
  int update;
  int verbose;
} Args = {
  .elfpath = NULL,
  .update = 0,
  .verbose = 0
};

bool verbose() { return Args.verbose == true; }

void dumpArgs()
{
  fprintf(stderr, "Args.elfpath=%s Args.update=%d "
	  "Args.verbose=%d\n",
	  Args.elfpath,
	  Args.update,
	  Args.verbose);
}

void usage(char *name)
{
  fprintf(stderr,
	  "USAGE: %s [-h] [-v] <elf file>\n" \
	  "     examine and edit symbol table of an elf file\n",
	  name);
}

bool
processArgs(int argc, char **argv)
{
    char opt;
    while ((opt = getopt(argc, argv, "hv")) != -1) {
      switch (opt) {
      case 'v':
	Args.verbose=1;
	break;
      case 'h':
	usage(argv[0]);
	return false;
      default:
	usage(argv[0]);
	return false;
      }
  } 
  
  if ((argc - optind) < 1) {
    usage(argv[0]);
    return false;
  }

  Args.elfpath = argv[optind];

  if (verbose()) dumpArgs();
  return true;
}

#define MAX_PATH 80

enum ELFSTATE { NONE=0, OPEN=1<<0, MAPPED=1<<1, STRSFOUND=1<<2, SYMSFOUND=1<<3, DIRTY=1<<4 };
typedef struct {
  char path[MAX_PATH];
  struct stat finfo;
  uint8_t *addr;
  uint8_t *strtbl;
  union {
    uint8_t *addr;
    Elf32_Sym *sym32;
    Elf64_Sym *sym64;
  } sym;
  int fd;
  int openflgs;
  int mmapflgs;
  int protflgs;
  enum ELFSTATE state;
} ELF;
  

int openElf(ELF *elf, char *path, int openflgs, int persist)
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
  
  return 1;
  
}

#define xstr(s) str(s)
#define str(s) #s

struct Desc {
  int val;
  char name[40];
  char desc[80];
};


#define DESC(v, d) { \
    .val = v, \
      .name = #v,				\
    .desc = d \
      }

struct Desc ETYPE[] = {
  DESC(ET_NONE, "An unknown type"),
  DESC(ET_REL, "A relocatable file"),
  DESC(ET_EXEC, "An executable file"),
  DESC(ET_DYN, "A shared object"),
  DESC(ET_CORE, "A core file"),
  DESC(-1, "")
};

struct Desc EICLASS[] = {
  DESC(ELFCLASSNONE, "This class is invalid"),
  DESC(ELFCLASS32, "32-bit architecture"),
  DESC(ELFCLASS64, "64-bit architecture"),
  DESC(-1, "")
};

struct Desc EIDATA[] = {
  DESC(ELFDATANONE, "Unknown data format."),
  DESC(ELFDATA2LSB, "Two's complement, little-endian."),
  DESC(ELFDATA2MSB, "Two's complement, big-endian."),
  DESC(-1,"")
};

struct Desc EIVERSION[] = {
  DESC(EV_NONE, "Invalid version."),
  DESC(EV_CURRENT, "Current version."),
  DESC(-1,"")
};

struct Desc EIOSABI[] = {
  DESC(ELFOSABI_SYSV, "UNIX System V ABI"),
  DESC(ELFOSABI_HPUX, "HP-UX ABI"),
  DESC(ELFOSABI_NETBSD, "NetBSD ABI"),
  DESC(ELFOSABI_LINUX, "Linux ABI"),
  DESC(ELFOSABI_SOLARIS, "Solaris ABI"),
  DESC(ELFOSABI_IRIX, "IRIX ABI"),
  DESC(ELFOSABI_FREEBSD, "FreeBSD ABI"),
  DESC(ELFOSABI_TRU64, "TRU64 UNIX ABI"),
  DESC(ELFOSABI_ARM, "ARM architecture ABI"),
  DESC(ELFOSABI_STANDALONE, "Stand-alone  (embedded)"),
  DESC(-1,"")
};


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
      
  
void 
printDesc(struct Desc *dtbl, int v, char *name, FILE *fp)
{
  for (int i=0; dtbl[i].val != -1; i++) {
    if (dtbl[i].val == v) {
      fprintf(fp, "%s : %d - %s : %s\n", name,
	      dtbl[i].val, dtbl[i].name, dtbl[i].desc);
      return;
    }
  }
  assert(0);
}

void 
printIdent(union EIDENT *ident, FILE *fp) {
  fprintf(fp, "ident:\n");
  fprintf(fp, "\tmagic[0]:0x%02hhx magic[1]:0x%02hhx"
	  " magic[2]:0x%02hhx magic[3]:0x%02hhx\n",
	  ident->values.m0, ident->values.m1,
	  ident->values.m2, ident->values.m3);
  printDesc(EICLASS, ident->values.class, "\tclass", fp);
  printDesc(EIDATA, ident->values.data, "\tdata", fp);
  printDesc(EIVERSION, ident->values.version, "\tversion", fp);
  printDesc(EIOSABI, ident->values.osabi, "\tosabi", fp);
  fprintf(fp, "\tabiversion: %d\n", ident->values.abiversion);
}

#if 0
dumptype
#endif
    
void printHdr(Elf64_Ehdr *hdr, FILE *fp)
{
  printIdent((union EIDENT *)&(hdr->e_ident), fp);
}

bool validElfIdent(ELF *elf) {
  Elf64_Ehdr *hdr = (void *)(elf->addr);
  union EIDENT *ident = (void *)&(hdr->e_ident);
  return (ident->values.m0 == ELFMAG0 &&
	  ident->values.m1 == ELFMAG1 &&
	  ident->values.m2 == ELFMAG2 &&
	  ident->values.m3 == ELFMAG3 );
}
int findStrTbl(ELF *elf)
{
  assert(elf);
  return 1;
}

int findSymTbl(ELF *elf)
{
  return 1;
}

int main(int argc, char **argv)
{
  ELF elf;

  if (!processArgs(argc, argv)) {
    return -1;
  }
  
  if (!openElf(&elf, Args.elfpath,
	       O_RDONLY, //open flags,
	       0        // persist flg
	       )) {
    return -1;
  }

  if (verbose()) {
    Elf64_Ehdr *hdr = (void *)elf.addr;
    printHdr(hdr,stderr);
  }

  if (!validElfIdent(&elf)) {
    fprintf(stderr, "ERROR: does not seem to be an elf file bad ident\n");
    return -1;
  }
  
  findStrTbl(&elf);
  findSymTbl(&elf);
  
  return 0;
}
