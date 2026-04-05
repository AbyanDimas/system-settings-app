#pragma once
#include <glib.h>

GHashTable *config_parser_read(const char *filepath);
gboolean config_parser_write(const char *filepath, GHashTable *config);
