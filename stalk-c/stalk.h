#ifndef STALK_H
#define STALK_H

#define YYDEBUG 1
//#define GC_DEBUG 1
//#define SYN_DEBUG 1

#ifdef GC_DEBUG
#define SL_GC_DEBUG(M, ...) fprintf(stderr, "[GC] " M "\n", ##__VA_ARGS__)
#else
#define SL_GC_DEBUG(M, ...)
#endif

// Whether to debug the syntax tree interpreter
#ifdef SYN_DEBUG
#define SL_SYN_DEBUG(M, ...) fprintf(stderr, "[SYN] %*s" M "\n", __depth, "", ##__VA_ARGS__)
#define SL_SYN_DEBUG_LEAVE __depth -= 1;
#define SL_SYN_DEBUG_ENTER __depth += 1;
#else
#define SL_SYN_DEBUG(M, ...)
#define SL_SYN_DEBUG_LEAVE
#define SL_SYN_DEBUG_ENTER
#endif

typedef int bool;
enum { false, true };

int main(int argc, char *argv[]);

#endif
