# simple_harness

Wiring harness editor and view in C. Based on [raylib](https://github.com/raysan5/raylib/).

![A harness demo gif](Demo.gif)

Edit simple_harness.c to point to a TTF font in your ``$HOME/bin`` directory and compile using your favourite C compiler.

Harness connectors and wiring are documented in text files. See ``test_harness.txt`` for an example.

Uses ``strsep(...)`` from ``<string.h>``; you might have to implement this yourself on some OS's.

To compile on ``macos``, have a look at the source code for ``compile.c``. Hints for other OS's are available from the raylib wiki.

License: GPL v3
