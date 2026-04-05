#include "page-startup.h"
#include <adwaita.h>

/* ── Startup Apps page ────────────────────────────────────────────────────
 * Reads exec-once lines from ~/.config/hypr/hyprland.conf and shows them.
 * ─────────────────────────────────────────────────────────────────────────*/

GtkWidget *page_startup_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Startup Apps");
    adw_preferences_page_set_icon_name(page, "system-run-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Autostart Programs");
    adw_preferences_group_set_description(group,
        "These programs start automatically with Hyprland. "
        "Edit ~/.config/hypr/hyprland.conf to change them.");
    adw_preferences_page_add(page, group);

    /* Read hyprland.conf */
    g_autofree char *conf_path = g_build_filename(
        g_get_home_dir(), ".config", "hypr", "hyprland.conf", NULL);
    g_autofree char *contents = NULL;
    g_autoptr(GError) err = NULL;

    if (!g_file_get_contents(conf_path, &contents, NULL, &err)) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Config not found");
        adw_action_row_set_subtitle(row, conf_path);
        adw_preferences_group_add(group, GTK_WIDGET(row));
        return GTK_WIDGET(page);
    }

    gboolean any = FALSE;
    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(lines[i]);
        /* Match exec-once = ... */
        if (!g_str_has_prefix(line, "exec-once")) continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        char *cmd = g_strstrip(eq + 1);
        if (strlen(cmd) == 0) continue;

        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        /* Use command name as title, full command as subtitle */
        g_auto(GStrv) parts = g_strsplit(cmd, " ", 2);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), parts[0]);
        adw_action_row_set_subtitle(row, cmd);
        adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("system-run-symbolic"));
        adw_preferences_group_add(group, GTK_WIDGET(row));
        any = TRUE;
    }

    if (!any) {
        AdwActionRow *empty_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty_row), "No exec-once entries found");
        adw_action_row_set_subtitle(empty_row, "Add exec-once = <command> to hyprland.conf");
        adw_preferences_group_add(group, GTK_WIDGET(empty_row));
    }

    return GTK_WIDGET(page);
}
