## Stalk-C

Work-in-progress faster C implementation based off of the reference Python/PyPy implementation.

### Getting Started

Just clone and run `make`. **Note:** If you're not on a Mac and haven't installed GNU Bison via Homebrew (`brew install bison`) then you'll need to edit the `Makefile` to change the location of your Bison installation (normally `bison` should be okay).

If it builds successfully then just run `./stalk source.sl` to run the interpreter on a source file.

### To-Do

- More bootstrapping of the interpreter (so you can do stuff)
- Slot caching (and slot cache reference lists)
- Type graphs for optimizations
- Look into more speed-ups in message sending (deeper ties between receivers and messages in simple cases if possible)
- Analyzations to tie into the `closure` message to speed up parent-scope look-ups and cache those lookups between repeated invocations of the same block
- Actually make object prefaces work (to tie into the `closure`/parent-scope system to create multi-scope linkages for super-fast look-ups)
- Actually make array literals work
- REPL-party!
- Refactor `syntax.c` (it's very, very messy)
- Do more syntax item hinting
