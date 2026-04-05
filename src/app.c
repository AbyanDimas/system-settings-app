#include "app.h"
#include "window.h"
#include <gtk/gtk.h>
#include <adwaita.h>

struct _OmarchySettingsApp {
    AdwApplication parent_instance;
};

G_DEFINE_TYPE(OmarchySettingsApp, omarchy_settings_app, ADW_TYPE_APPLICATION)

static void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();

    /* Try loading from source dir first (dev mode), then installed path */
    const char *local_css = "src/style.css";
    if (g_file_test(local_css, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(provider, local_css);
    } else if (g_file_test("../src/style.css", G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(provider, "../src/style.css");
    } else {
        /* Installed location */
        g_autofree char *path = g_build_filename(
            g_get_user_data_dir(), "system-settings", "style.css", NULL);
        if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
            /* Fallback: same dir as binary */
            path = g_strdup("style.css");
        }
        gtk_css_provider_load_from_path(provider, path);
    }

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

static void omarchy_settings_app_activate(GApplication *app) {
    OmarchySettingsApp *self = OMARCHY_SETTINGS_APP(app);
    GtkWindow *win = gtk_application_get_active_window(GTK_APPLICATION(self));

    if (!win) {
        win = GTK_WINDOW(omarchy_settings_window_new(self));
    }

    gtk_window_present(win);
}

#include "utils/tool-detector.h"

static void omarchy_settings_app_startup(GApplication *app) {
    G_APPLICATION_CLASS(omarchy_settings_app_parent_class)->startup(app);
    omarchy_detect_system_caps();
    load_css();
}

static void omarchy_settings_app_class_init(OmarchySettingsAppClass *klass) {
    GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
    app_class->activate = omarchy_settings_app_activate;
    app_class->startup = omarchy_settings_app_startup;
}

static void omarchy_settings_app_init(OmarchySettingsApp *self) {
    (void)self;
    g_set_application_name("System Settings");
}

OmarchySettingsApp *omarchy_settings_app_new(void) {
    return g_object_new(OMARCHY_TYPE_SETTINGS_APP,
                        "application-id", "com.system.Settings",
                        "flags", G_APPLICATION_DEFAULT_FLAGS,
                        NULL);
}
