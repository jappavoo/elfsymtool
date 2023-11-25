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

      
struct Desc EMACHINE[] = {
  DESC(EM_NONE, "An unknown machine"),
  DESC(EM_M32, "AT&T WE 32100"),
  DESC(EM_SPARC, "Sun Microsystems SPARC"),
  DESC(EM_386, "Intel 80386"),
  DESC(EM_68K, "Motorola 68000"),
  DESC(EM_88K, "Motorola 88000"),
  DESC(EM_860, "Intel 80860"),
  DESC(EM_MIPS, "MIPS RS3000 (big-endian only)"),
  DESC(EM_PARISC, "HP/PA"),
  DESC(EM_SPARC32PLUS, "SPARC with enhanced instruction set"),
  DESC(EM_PPC, "PowerPC"),
  DESC(EM_PPC64, "PowerPC 64-bit"),
  DESC(EM_S390, "IBM S/390"),
  DESC(EM_ARM, "Advanced RISC Machines"),
  DESC(EM_SH, "Renesas SuperH"),
  DESC(EM_SPARCV9, "SPARC v9 64-bit"),
  DESC(EM_IA_64, "Intel Itanium"),
  DESC(EM_X86_64, "AMD x86-64"),
  DESC(EM_VAX, "DEC Vax"),
  DESC(-1,"")
};

struct Desc EVERSION[] = {
  DESC(EV_NONE, "Invalid version"),
  DESC(EV_CURRENT, "Current version"),
  DESC(-1,"")
};

struct Desc SHTYPE[] = {
  DESC(SHT_NULL, "Section header table entry unused"),
  DESC(SHT_PROGBITS, "Program data"),
  DESC(SHT_SYMTAB, "Symbol table"),
  DESC(SHT_STRTAB, "String table"),
  DESC(SHT_RELA, "Relocation entries with addends"),
  DESC(SHT_HASH, "Symbol hash table"),
  DESC(SHT_DYNAMIC, "Dynamic linking information"),
  DESC(SHT_NOTE, "Notes"),
  DESC(SHT_NOBITS, "Program space with no data (bss)"),
  DESC(SHT_REL, "Relocation entries, no addends"),
  DESC(SHT_SHLIB, "Reserved"),
  DESC(SHT_DYNSYM, "Dynamic linker symbol table"),
  DESC(SHT_INIT_ARRAY, "Array of constructors"),
  DESC(SHT_FINI_ARRAY, "Array of destructors"),
  DESC(SHT_PREINIT_ARRAY, "Array of pre-constructors"),
  DESC(SHT_GROUP, "Section group"),
  DESC(SHT_SYMTAB_SHNDX, "Extended section indices"),
  DESC(SHT_NUM, "Number of defined types."),
  DESC(SHT_LOOS, "Start OS-specific."),
  DESC(SHT_GNU_ATTRIBUTES, "Object attributes."),
  DESC(SHT_GNU_HASH, "GNU-style hash table."),
  DESC(SHT_GNU_LIBLIST, "Prelink library list"),
  DESC(SHT_CHECKSUM, "Checksum for DSO content."),
  DESC(SHT_LOSUNW, "Sun-specific low bound."),
  DESC(SHT_SUNW_move, ""),
  DESC(SHT_SUNW_COMDAT, ""),
  DESC(SHT_SUNW_syminfo, ""),
  DESC(SHT_GNU_verdef, "Version definition section."),
  DESC(SHT_GNU_verneed, "Version needs section."),
  DESC(SHT_GNU_versym, "Version symbol table."),
  DESC(SHT_HISUNW, "Sun-specific high bound."),
  DESC(SHT_HIOS, "End OS-specific type"),
  DESC(SHT_LOPROC, "Start of processor-specific"),
  DESC(SHT_HIPROC, "End of processor-specific"),
  DESC(SHT_LOUSER, "Start of application-specific"),
  DESC(SHT_HIUSER, "End of application-specific"),
  DESC(-1,"")
};

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
  printDesc(EIVERSION, ident->values.version, "\tver", fp);
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
