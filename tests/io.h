#ifndef IO_H
#define IO_H
/* Stub for test builds. When force-included before engine.c, the IO_H guard
 * prevents the real io.h (and its <curses.h> pull-in) from being processed,
 * keeping bool/struct layouts consistent across all test translation units. */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7
#endif
