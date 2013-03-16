#ifndef STALK_H
#define STALK_H

#define YYDEBUG 1
//#define GC_DEBUG 1

#ifdef GC_DEBUG
#define SL_GC_DEBUG(M, ...) fprintf(stderr, "[GC] " M "\n", ##__VA_ARGS__)
#else
#define SL_GC_DEBUG(M, ...)
#endif

typedef int bool;
enum { false, true };

int main(int argc, char *argv[]);

#endif
