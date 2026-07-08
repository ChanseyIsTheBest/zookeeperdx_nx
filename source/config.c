/* config.c -- simple configuration parser
 *
 * Copyright (C) 2021 Andy Nguyen, fgsfds
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "util.h"

#define CONFIG_VARS \
  CONFIG_VAR_INT(screen_width); \
  CONFIG_VAR_INT(screen_height); \
  CONFIG_VAR_INT(language); \
  CONFIG_VAR_INT(portrait);

Config config;
static int config_needs_rewrite = 0;

// actual screen size that is in use right now
int screen_width = 720;   /* fbstub45 PORTRAIT (stable) */
int screen_height = 1280;

static inline void parse_var(const char *name, const char *value) {
  // retired options -> drop them and rewrite the file without them
  if (!strcmp(name, "touchscreen") || !strcmp(name, "controller_cursor") ||
      !strcmp(name, "show_fps") || !strcmp(name, "widescreen")) {
    config_needs_rewrite = 1;
    return;
  }

  #define CONFIG_VAR_INT(var) if (!strcmp(name, #var)) { config.var = atoi(value); return; }
  #define CONFIG_VAR_FLOAT(var) if (!strcmp(name, #var)) { config.var = atof(value); return; }
  #define CONFIG_VAR_STR(var) if (!strcmp(name, #var)) { strlcpy(config.var, value, sizeof(config.var)); return; }
  CONFIG_VARS
  #undef CONFIG_VAR_INT
  #undef CONFIG_VAR_FLOAT
  #undef CONFIG_VAR_STR
}

int read_config(const char *file) {
  char line[1024] = { 0 };

  memset(&config, 0, sizeof(Config));
  config_needs_rewrite = 0;
  config.screen_width = -1; // auto
  config.screen_height = -1;
  config.language = LANG_EN;
  config.portrait = 1;   /* rotate 90 CW by default (hold right Joy-Con up) */

  FILE *f = fopen(file, "r");
  if (f == NULL)
    return -1;

  // parse lines of the forms
  // <spaces> # <whatever> \n
  // <spaces> NAME <spaces> VALUE <spaces> \n
  do {
    char *name = NULL, *value = NULL, *tmp = NULL;
    if (fgets(line, sizeof(line), f) != NULL) {
      name = line;
      // trim name
      while (*name && isspace((int)*name)) ++name;
      if (name[0] == '#') continue; // skip comments
      for (tmp = name; *tmp && !isspace((int)*tmp); ++tmp);
      // if tmp points to the end of the string, there's no value to parse
      if (*tmp != 0) {
        *tmp = 0;
        // value is next; trim value
        for (value = tmp + 1; *value && isspace((int)*value); ++value);
        for (tmp = value + strlen(value) - 1; isspace((int)*tmp); --tmp) *tmp = 0;
        // got key value pair
        parse_var(name, value);
      }
    }
  } while (!feof(f));

  fclose(f);

  return config_needs_rewrite ? 1 : 0;
}

int write_config(const char *file) {
  FILE *f = fopen(file, "w");
  if (f == NULL)
    return -1;

  fprintf(f,
    "# zookeeper_nx configuration -- lines are \"name value\"; # starts a comment\n"
    "#\n"
    "# portrait -- the render is rotated 90 degrees to fill the screen (hold the\n"
    "#             console rotated to play):\n"
    "#             1 = rotate clockwise (default, right Joy-Con up)\n"
    "#             2 = rotate counter-clockwise (left Joy-Con up)\n"
    "#             0 = no rotation (stretched 16:9)\n"
    "# language -- in-game language (applies on next launch):\n"
    "#             2 = English (default)\n"
    "#             1 = Japanese\n"
    "#             0 = follow the Switch system language\n"
    "# screen_width / screen_height -- render size; -1 = auto\n"
    "\n");

  #define CONFIG_VAR_INT(var) fprintf(f, "%s %d\n", #var, config.var)
  #define CONFIG_VAR_FLOAT(var) fprintf(f, "%s %g\n", #var, config.var)
  #define CONFIG_VAR_STR(var) if (config.var[0]) fprintf(f, "%s %s\n", #var, config.var)
  CONFIG_VARS
  #undef CONFIG_VAR_INT
  #undef CONFIG_VAR_FLOAT
  #undef CONFIG_VAR_STR

  fclose(f);

  return 0;
}
