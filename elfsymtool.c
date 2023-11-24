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


#if 0
void ELFprintSections(ELF *elf, FILE *fp)
{
  ElfN_Ehdr *hdr = (void *)elf->addr;
  ElfN_Shdr *shdr = (void *)(elf->addr + NF(elf,hdr,e_shoff));
  int n = NF(elf,hdr,e_shnum);

  for (int i=0; i<n; i++) {
    fprintf(stderr, "section[%d]:",i);
    printDesc(SHTYPE, NF(elf,shdrs[i],sh_type), "sh_type", fp);
    shdr = NPTRADD(elf,shdr,1)
  }
  
}

char *
ELFString(ELF *elf, int i) {
  return "";
}

void ELFprintSym(ELF *elf, ElfN_Sym *sym, FILE *fp)
{
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
}
#endif


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
  
  if (!ELFopen(&elf, Args.elfpath,
	       O_RDONLY, //open flags,
	       0        // persist flg
	       )) {
    return -1;
  }

  if (verbose()) {
    ELFprintHdr(&elf, stderr);
    //    ELFprintSections(&elf, stderr);
  }
  
  findStrTbl(&elf);
  findSymTbl(&elf);
  
  return 0;
}
