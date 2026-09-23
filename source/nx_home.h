/* nx_home.h -- the game folder, decided at launch (nx_home.c). */
#ifndef NX_HOME_H
#define NX_HOME_H
#include <stddef.h>

#define NX_DEFAULT_HOME "sdmc:/switch/zookeeper"   /* when the launcher passes no usable path */

const char *nx_home(void);          /* e.g. "sdmc:/switch/zoo_nx" -- no trailing slash */
const char *nx_home_source(void);   /* why that folder, for the boot log */
void nx_home_init(int argc, char **argv);          /* first thing in main() */
int  nx_home_from_argv0(const char *argv0, char *out, size_t n);  /* pure; host-tested */
#endif
