#include "page-keybinds.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Keybinds page ────────────────────────────────────────────────────────
 * Reads bind entries from hyprland.conf and displays them.
 * ─────────────────────────────────────────────────────────────────────────*/

GtkWidget *page_keybinds_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Keybinds");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-keyboard-shortcuts-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Hyprland Keybinds");
    adw_preferences_group_set_description(group,
        "Read from ~/.config/hypr/hyprland.conf");
    adw_preferences_page_add(page, group);

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
        /* Match "bind = MODS, KEY, dispatcher, arg" */
        if (!g_str_has_prefix(line, "bind")) continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        char *rest = g_strstrip(eq + 1);
        if (strlen(rest) == 0) continue;

        /* Split: MODS, KEY, DISPATCHER, ...ARG */
        g_auto(GStrv) parts = g_strsplit(rest, ",", 4);
        if (!parts[0] || !parts[1] || !parts[2]) continue;

        const char *mods = g_strstrip(parts[0]);
        const char *key = g_strstrip(parts[1]);
        const char *disp = g_strstrip(parts[2]);
        const char *arg = parts[3] ? g_strstrip(parts[3]) : "";

        /* Build label like "SUPER + T" */
        g_autofree char *hotkey;
        if (strlen(mods) > 0)
            hotkey = g_strdup_printf("%s + %s", mods, key);
        else
            hotkey = g_strdup(key);

        /* Build subtitle like "exec kitty" */
        g_autofree char *subtitle = g_strdup_printf("%s %s", disp, arg);

        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), hotkey);
        adw_action_row_set_subtitle(row, g_strstrip(subtitle));

        /* Badge label widget */
        GtkWidget *badge = gtk_label_new(hotkey);
        gtk_widget_add_css_class(badge, "keybind-badge");
        adw_action_row_add_suffix(row, badge);

        adw_preferences_group_add(group, GTK_WIDGET(row));
        any = TRUE;
    }

    if (!any) {
        AdwActionRow *empty_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty_row), "No bind entries found");
        adw_action_row_set_subtitle(empty_row, "Add bind = SUPER, T, exec, kitty to hyprland.conf");
        adw_preferences_group_add(group, GTK_WIDGET(empty_row));
    }

    return GTK_WIDGET(page);
}
