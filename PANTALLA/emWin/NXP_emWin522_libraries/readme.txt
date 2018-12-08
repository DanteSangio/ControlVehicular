These emWin library files were built using:

- arm-none-eabi-gcc gcc version 4.6.2 (LPCXPresso 5.1.0 by Code Red).
- IAR ANSI C/C++ Compiler V6.50.1.4415/W32 for ARM
  Copyright 1999-2012 IAR Systems AB.
- ARM C/C++ Compiler, 5.02 [Build 28] [MDK-ARM Professional]
- Visual C++ 2010 Express edition

There is one set of library files for each of the 5 ARM cores and one for the x86 core.

Library files that include '_d5' are built with GUI_DEBUG_LEVEL=5. Linking with these libraries will cause diagnostic messages for log entries, warnings, and entries to be produced.
The file GUI_Debug.h contains a description of these debug levels.
