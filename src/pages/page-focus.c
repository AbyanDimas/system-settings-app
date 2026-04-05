#include "page-focus.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Focus page ───────────────────────────────────────────────────────────
 * Ties into mako DND and optional hyprland animation disable.
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_focus_mode_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    if (active) {
        /* DND + disable animations */
        subprocess_run_sync("makoctl mode -a do-not-disturb", &err);
        g_clear_error(&err);
        subprocess_run_sync("hyprctl keyword animations:enabled 0", &err);
    } else {
        subprocess_run_sync("makoctl mode -r do-not-disturb", &err);
        g_clear_error(&err);
        subprocess_run_sync("hyprctl keyword animations:enabled 1", &err);
    }
}

GtkWidget *page_focus_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Focus");
    adw_preferences_page_set_icon_name(page, "applications-utilities-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Focus Mode");
    adw_preferences_group_set_description(group,
        "Silences notifications and disables animations for distraction-free work.");
    adw_preferences_page_add(page, group);

    /* Check current mako mode */
    g_autoptr(GError) err = NULL;
    g_autofree char *mode_out = subprocess_run_sync("makoctl mode", &err);
    gboolean focus_on = mode_out && strstr(mode_out, "do-not-disturb") != NULL;

    AdwSwitchRow *focus_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(focus_row), "Focus Mode");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(focus_row),
        "Mutes notifications + disables Hyprland animations");
    adw_switch_row_set_active(focus_row, focus_on);
    g_signal_connect(focus_row, "notify::active", G_CALLBACK(on_focus_mode_toggled), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(focus_row));

    return GTK_WIDGET(page);
}
