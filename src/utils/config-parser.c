#include "config-parser.h"
#include <gio/gio.h>
#include <string.h>

GHashTable *config_parser_read(const char *filepath) {
    GHashTable *config = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_autoptr(GFile) file = g_file_new_for_path(filepath);
    
    if (!g_file_query_exists(file, NULL)) return config;

    g_autoptr(GFileInputStream) is = g_file_read(file, NULL, NULL);
    if (!is) return config;

    g_autoptr(GDataInputStream) dis = g_data_input_stream_new(G_INPUT_STREAM(is));
    char *line;
    gsize length;
    
    while ((line = g_data_input_stream_read_line(dis, &length, NULL, NULL)) != NULL) {
        g_strstrip(line);
        if (line[0] == '#' || line[0] == '\0') {
            g_free(line);
            continue;
        }

        char **parts = g_strsplit(line, "=", 2);
        if (parts[0] && parts[1]) {
            g_hash_table_insert(config, g_strdup(g_strstrip(parts[0])), g_strdup(g_strstrip(parts[1])));
        }
        g_strfreev(parts);
        g_free(line);
    }
    return config;
}

gboolean config_parser_write(const char *filepath, GHashTable *config) {
    g_autoptr(GFile) file = g_file_new_for_path(filepath);
    g_autoptr(GFileOutputStream) os = g_file_replace(file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    if (!os) return FALSE;

    g_autoptr(GDataOutputStream) dos = g_data_output_stream_new(G_OUTPUT_STREAM(os));
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, config);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        g_autofree char *line = g_strdup_printf("%s=%s\n", (char *)key, (char *)value);
        g_data_output_stream_put_string(dos, line, NULL, NULL);
    }
    
    return TRUE;
}
