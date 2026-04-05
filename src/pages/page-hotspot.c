#include "page-hotspot.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Hotspot page ─────────────────────────────────────────────────────────
 * Reads the "Hotspot" nmcli connection, shows SSID+password if it exists,
 * and provides a toggle to bring the connection up/down.
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_hotspot_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    if (active) {
        subprocess_run_sync("nmcli con up Hotspot", &err);
    } else {
        subprocess_run_sync("nmcli con down Hotspot", &err);
    }
}

GtkWidget *page_hotspot_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Hotspot");
    adw_preferences_page_set_icon_name(page, "network-wireless-hotspot-symbolic");

    /* ── Toggle group ────────────────────────────────────────────────── */
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Personal Hotspot");
    adw_preferences_page_add(page, group);

    /* Check if hotspot is currently active */
    g_autoptr(GError) err = NULL;
    g_autofree char *active_out = subprocess_run_sync(
        "nmcli -t -f NAME,TYPE,STATE con show --active", &err);
    gboolean hotspot_active = active_out &&
        (strstr(active_out, "Hotspot") != NULL || strstr(active_out, "hotspot") != NULL);

    AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Wi-Fi Hotspot");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Share your connection with other devices");
    adw_switch_row_set_active(row, hotspot_active);
    g_signal_connect(row, "notify::active", G_CALLBACK(on_hotspot_toggled), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(row));

    /* ── Info group ──────────────────────────────────────────────────── */
    AdwPreferencesGroup *info_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(info_group, "Hotspot Details");
    adw_preferences_page_add(page, info_group);

    /* Read SSID from nmcli */
    g_autoptr(GError) err2 = NULL;
    g_autofree char *ssid_out = subprocess_run_sync(
        "nmcli -t -f 802-11-wireless.ssid con show Hotspot", &err2);
    const char *ssid = "Hotspot";
    if (ssid_out) {
        char *colon = strchr(ssid_out, ':');
        if (colon) ssid = g_strstrip(colon + 1);
    }

    AdwActionRow *ssid_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ssid_row), "Network Name (SSID)");
    adw_action_row_set_subtitle(ssid_row, ssid_out ? ssid : "Hotspot (not configured)");
    adw_preferences_group_add(info_group, GTK_WIDGET(ssid_row));

    /* Read password */
    g_autoptr(GError) err3 = NULL;
    g_autofree char *pass_out = subprocess_run_sync(
        "nmcli -s -t -f 802-11-wireless-security.psk con show Hotspot", &err3);
    const char *pass = "—";
    if (pass_out) {
        char *colon = strchr(pass_out, ':');
        if (colon) pass = g_strstrip(colon + 1);
    }

    AdwPasswordEntryRow *pass_row = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pass_row), "Password");
    gtk_editable_set_text(GTK_EDITABLE(pass_row), pass_out ? pass : "");
    adw_preferences_group_add(info_group, GTK_WIDGET(pass_row));

    return GTK_WIDGET(page);
}
