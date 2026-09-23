/* nx_home.c -- the game folder: wherever the .nro was launched from.
 *
 * The Homebrew Menu passes the .nro's own path as argv[0], for example
 * "sdmc:/switch/zoo_nx/zookeeper_nx.nro"; that folder holds the game files,
 * config.txt, the saves and debug.log. Any folder name works. A launcher that
 * passes no usable path -- none at all, a bare file name, another device, or
 * the SD card's root, which is no game folder -- gets NX_DEFAULT_HOME.
 *
 * Decided ONCE, as the first statement of main(), before the first log line:
 * debug.log opens lazily at the first debugPrintf, in this folder. MIT.
 */
#include <stdio.h>
#include <string.h>
#include "nx_home.h"

static char s_home[256] = NX_DEFAULT_HOME;
static const char *s_why = "default -- not decided yet";

const char *nx_home(void) { return s_home; }
const char *nx_home_source(void) { return s_why; }

int nx_home_from_argv0(const char *a0, char *out, size_t n) {
  char buf[512];
  if (!a0 || !*a0) return 0;
  if (!strncmp(a0, "sdmc:/", 6)) {
    if (strlen(a0) >= sizeof buf) return 0;
    snprintf(buf, sizeof buf, "%s", a0);
  } else if (a0[0] == '/') {                 /* some launchers leave off the device */
    if (strlen(a0) + 5 >= sizeof buf) return 0;
    snprintf(buf, sizeof buf, "sdmc:%s", a0);
  } else {
    return 0;                                /* a bare name, or another device */
  }
  char *slash = strrchr(buf, '/');
  *slash = '\0';                             /* drop the .nro's own name */
  size_t len = strlen(buf);
  while (len > 5 && buf[len - 1] == '/') buf[--len] = '\0';
  if (len <= 5) return 0;                    /* "sdmc:" -- the SD root is no game folder */
  if (len >= n) return 0;
  memcpy(out, buf, len + 1);
  return 1;
}

void nx_home_init(int argc, char **argv) {
  char h[sizeof s_home];
  if (argc > 0 && argv && nx_home_from_argv0(argv[0], h, sizeof h)) {
    snprintf(s_home, sizeof s_home, "%s", h);
    s_why = "the .nro's own folder";
  } else {
    snprintf(s_home, sizeof s_home, "%s", NX_DEFAULT_HOME);   /* the report never disagrees with the folder */
    s_why = "default -- the launcher passed no usable .nro path";
  }
}
