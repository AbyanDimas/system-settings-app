#include "page-privacy.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Privacy & Security page ──────────────────────────────────────────────
 * Screen lock, hypridle config, and privacy controls.
 * ─────────────────────────────────────────────────────────────────────────*/

GtkWidget *page_privacy_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Privacy & Security");
    adw_preferences_page_set_icon_name(page, "security-high-symbolic");

    /* ── Screen Lock ─────────────────────────────────────────────────── */
    AdwPreferencesGroup *lock_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(lock_group, "Screen Lock");
    adw_preferences_group_set_description(lock_group, "Managed by hypridle / hyprlock.");
    adw_preferences_page_add(page, lock_group);

    /* Check if hypridle config exists */
    g_autofree char *idle_conf = g_build_filename(
        g_get_home_dir(), ".config", "hypr", "hypridle.conf", NULL);
    gboolean has_idle_conf = g_file_test(idle_conf, G_FILE_TEST_EXISTS);

    AdwActionRow *lock_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(lock_row), "hypridle Config");
    adw_action_row_set_subtitle(lock_row,
        has_idle_conf ? idle_conf : "~/.config/hypr/hypridle.conf not found");
    adw_action_row_add_suffix(lock_row, gtk_image_new_from_icon_name(
        has_idle_conf ? "emblem-ok-symbolic" : "dialog-warning-symbolic"));
    adw_preferences_group_add(lock_group, GTK_WIDGET(lock_row));

    AdwActionRow *lock_now_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(lock_now_row), "Lock Screen Now");
    adw_action_row_set_subtitle(lock_now_row, "Runs hyprlock");
    adw_action_row_add_suffix(lock_now_row, gtk_image_new_from_icon_name("go-next-symbolic"));
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(lock_now_row), TRUE);
    adw_preferences_group_add(lock_group, GTK_WIDGET(lock_now_row));

    /* ── Privacy ─────────────────────────────────────────────────────── */
    AdwPreferencesGroup *priv_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(priv_group, "Privacy");
    adw_preferences_page_add(page, priv_group);

    AdwActionRow *loc_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(loc_row), "Location Services");
    adw_action_row_set_subtitle(loc_row, "No location daemon configured (geoclue2 not required)");
    adw_preferences_group_add(priv_group, GTK_WIDGET(loc_row));

    return GTK_WIDGET(page);
}
