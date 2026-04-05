#include "subprocess.h"
#include <gio/gio.h>

void subprocess_run_async(const char *command, GAsyncReadyCallback callback, gpointer user_data) {
    GError *error = NULL;
    char **argv;
    if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
        if (callback) callback(NULL, NULL, user_data); // Should properly handle
        g_error_free(error);
        return;
    }

    GSubprocess *proc = g_subprocess_newv((const char * const *)argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &error);
    g_strfreev(argv);
    if (!proc) {
        if (callback) callback(NULL, NULL, user_data);
        g_error_free(error);
        return;
    }

    g_subprocess_communicate_utf8_async(proc, NULL, NULL, callback, user_data);
}

char *subprocess_run_sync(const char *command, GError **error) {
    char **argv;
    if (!g_shell_parse_argv(command, NULL, &argv, error)) {
        return NULL;
    }

    GSubprocess *proc = g_subprocess_newv((const char * const *)argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, error);
    g_strfreev(argv);
    if (!proc) return NULL;

    char *stdout_buf = NULL;
    g_subprocess_communicate_utf8(proc, NULL, NULL, &stdout_buf, NULL, error);
    g_object_unref(proc);
    
    return stdout_buf;
}
