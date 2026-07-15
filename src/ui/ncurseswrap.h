#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
#ifdef DEBIAN
#include <ncursesw/curses.h>
#elif defined(NCURSES_SUBDIR)
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif
