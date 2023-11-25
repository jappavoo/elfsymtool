#ifndef __DESC_H__
#define __DESC_H__

// elf decoding tables
#define xstr(s) str(s)
#define str(s) #s

struct Desc {
  int val;
  char name[40];
  char desc[256];
};

#define DESC(v, d) {				\
      .val = v,		          		\
      .name = #v,				\
      .desc = d					\
      }

#endif
