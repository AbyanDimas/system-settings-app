#include "page-notifications.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Notifications page ───────────────────────────────────────────────────
 * Reads current DND state from makoctl and toggles it.
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_dnd_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    if (active) {
        subprocess_run_sync("makoctl mode -a do-not-disturb", &err);
    } else {
        subprocess_run_sync("makoctl mode -r do-not-disturb", &err);
    }
}

GtkWidget *page_notifications_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Notifications");
    adw_preferences_page_set_icon_name(page, "preferences-system-notifications-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Focus");
    adw_preferences_page_add(page, group);

    /* Read current mako mode */
    g_autoptr(GError) err = NULL;
    g_autofree char *mode_out = subprocess_run_sync("makoctl mode", &err);
    gboolean dnd_active = mode_out && strstr(mode_out, "do-not-disturb") != NULL;

    AdwSwitchRow *dnd_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dnd_row), "Do Not Disturb");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(dnd_row), "Silence all notifications");
    adw_switch_row_set_active(dnd_row, dnd_active);
    g_signal_connect(dnd_row, "notify::active", G_CALLBACK(on_dnd_toggled), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(dnd_row));

    /* ── Mako config info ──────────────────────────────────────────── */
    AdwPreferencesGroup *info_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(info_group, "Notification Daemon");
    adw_preferences_page_add(page, info_group);

    AdwActionRow *daemon_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(daemon_row), "Mako");
    adw_action_row_set_subtitle(daemon_row,
        err ? "mako not running or not found" : "Active notification daemon");
    adw_action_row_add_suffix(daemon_row,
        gtk_image_new_from_icon_name(err ? "dialog-warning-symbolic" : "emblem-ok-symbolic"));
    adw_preferences_group_add(info_group, GTK_WIDGET(daemon_row));

    return GTK_WIDGET(page);
}
