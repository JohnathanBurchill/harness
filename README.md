# harness

Wiring harness viewer in C and raylib.

![A harness demo gif](Demo.gif)

Put a TTF font in your ``$HOME/bin`` directory, along with the ``harness`` binary. 

Harness connectors and wiring are documented in text files. See ``test_harness.txt`` for an example.

Uses ``strsep(...)`` from ``<string.h>``; you might have to implement this yourself on some OS's.

To compile, have a look at the source code for ``compile.c``.

License: GPL v3
