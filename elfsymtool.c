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


int processSymTbls(ELF *elf)
{
  ElfN_Shdr *stshdr;
  int idx;
  assert(elf);

  if (verbose()) fprintf(stderr, "Symbol Tables:\n");
  for (idx=0, ELFnextSymTbl(elf, &idx, &stshdr);
       stshdr != NULL;
       idx++, ELFnextSymTbl(elf, &idx, &stshdr))  {
    if (verbose()) {
      fprintf(stderr, "Found Sym Section: %d\n", idx);
      ELFprintShdr(elf, stshdr, stderr);
    }
    ELFprintSymTbl(elf, stshdr, stderr);
  }
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
    ELFprintSections(&elf, stderr);
  }
  
  processSymTbls(&elf);
  
  return 0;
}
