#include "page-shaders.h"
#include "../utils/subprocess.h"
#include <adwaita.h>
#include <gio/gio.h>

/* ── Shaders page ─────────────────────────────────────────────────────────
 * Lists GLSL files from ~/.config/hypr/shaders/ and applies them live.
 * ─────────────────────────────────────────────────────────────────────────*/

static void apply_shader(const char *filepath) {
    g_autoptr(GError) err = NULL;
    if (!filepath || strlen(filepath) == 0) {
        subprocess_run_sync("hyprctl keyword decoration:screen_shader \"[[EMPTY]]\"", &err);
    } else {
        g_autofree char *cmd = g_strdup_printf("hyprctl keyword decoration:screen_shader \"%s\"", filepath);
        subprocess_run_sync(cmd, &err);
    }
}

static void on_shader_row_activated(AdwActionRow *row, gpointer user_data) {
    (void)user_data;
    const char *filepath = g_object_get_data(G_OBJECT(row), "filepath");
    apply_shader(filepath);
    
    /* Show toast */
    const char *title = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    g_autofree char *msg = g_strdup_printf("Shader applied: %s", title);
    
    /* Find parent toast overlay via widget tree is complex, let's just apply for now. */
}

GtkWidget *page_shaders_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Screen Shaders");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-display-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Available Shaders");
    adw_preferences_group_set_description(group, "Select a shader pattern. Applies immediately.");
    adw_preferences_page_add(page, group);

    /* None option */
    AdwActionRow *none_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(none_row), "None (No Shader)");
    adw_action_row_add_prefix(none_row, gtk_image_new_from_icon_name("edit-clear-all-symbolic"));
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(none_row), TRUE);
    g_object_set_data_full(G_OBJECT(none_row), "filepath", g_strdup(""), g_free);
    g_signal_connect(none_row, "activated", G_CALLBACK(on_shader_row_activated), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(none_row));

    /* Load from ~/.config/hypr/shaders/ */
    g_autofree char *dir_path = g_build_filename(g_get_home_dir(), ".config", "hypr", "shaders", NULL);
    g_autoptr(GFile) dir = g_file_new_for_path(dir_path);
    g_autoptr(GFileEnumerator) enumerator = g_file_enumerate_children(dir,
        G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (enumerator) {
        GFileInfo *info;
        while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
            const char *name = g_file_info_get_name(info);
            if (g_str_has_suffix(name, ".glsl")) {
                g_autofree char *filepath = g_build_filename(dir_path, name, NULL);
                g_autofree char *display_name = g_strndup(name, strlen(name) - 5); /* Remove .glsl */

                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), display_name);
                adw_action_row_add_prefix(row, gtk_image_new_from_icon_name("applications-graphics-symbolic"));
                gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
                g_object_set_data_full(G_OBJECT(row), "filepath", g_strdup(filepath), g_free);
                g_signal_connect(row, "activated", G_CALLBACK(on_shader_row_activated), NULL);
                
                adw_preferences_group_add(group, GTK_WIDGET(row));
            }
            g_object_unref(info);
        }
    }

    return GTK_WIDGET(page);
}
