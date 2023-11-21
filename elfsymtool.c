#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <getopt.h>

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

typedef struct {
  char path[MAX_PATH];
  int fd;
  uint8_t *addr;
  uint8_t *strtbl;
  union {
    uint8_t *addr;
    Elf32_Sym *sym32;
    Elf64_Sym *sym64;
  } sym;
} ELF;
  
int openElf(char *path, ELF *elf)
{
  return 1;
}

int findStrTbl(ELF *elf)
{
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
  
  openElf(argv[1], &elf);
  findStrTbl(&elf);
  findSymTbl(&elf);
  
  return 0;
}
