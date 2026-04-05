#pragma once
#include <glib.h>
#include <gio/gio.h>

void subprocess_run_async(const char *command, GAsyncReadyCallback callback, gpointer user_data);
char *subprocess_run_sync(const char *command, GError **error);
