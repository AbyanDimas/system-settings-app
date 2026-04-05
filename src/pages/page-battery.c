#include "page-battery.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>
#include <stdio.h>

/* ── Battery page ─────────────────────────────────────────────────────────
 * Reads /sys/class/power_supply/BAT* for real battery info.
 * ─────────────────────────────────────────────────────────────────────────*/

static char *read_sys_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    char buf[256] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    /* Strip newline */
    int len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return g_strdup(buf);
}

static void on_power_profile_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = adw_combo_row_get_selected(row);
    const char *profile = "balanced";
    if (selected == 0) profile = "power-saver";
    if (selected == 1) profile = "balanced";
    if (selected == 2) profile = "performance";

    g_autofree char *cmd = g_strdup_printf("powerprofilesctl set %s", profile);
    subprocess_run_async(cmd, NULL, NULL);
}


GtkWidget *page_battery_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Battery & Power");
    adw_preferences_page_set_icon_name(page, "battery-good-symbolic");

    /* ── Power Profiles ─────────────────────────────────────────────── */
    AdwPreferencesGroup *power_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(power_group, "Power Management");
    adw_preferences_group_set_description(power_group, "Configure system performance modes (requires power-profiles-daemon)");
    adw_preferences_page_add(page, power_group);

    AdwComboRow *profile_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(profile_row), "Power Profile");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(profile_row), "Optimize battery life or performance");

    GtkStringList *model = gtk_string_list_new(NULL);
    gtk_string_list_append(model, "Power Saver");
    gtk_string_list_append(model, "Balanced");
    gtk_string_list_append(model, "Performance");
    adw_combo_row_set_model(profile_row, G_LIST_MODEL(model));

    /* Get current profile */
    g_autoptr(GError) profile_err = NULL;
    g_autofree char *current_profile = subprocess_run_sync("powerprofilesctl get", &profile_err);
    if (current_profile) {
        g_strstrip(current_profile);
        if (strcmp(current_profile, "power-saver") == 0) adw_combo_row_set_selected(profile_row, 0);
        else if (strcmp(current_profile, "balanced") == 0) adw_combo_row_set_selected(profile_row, 1);
        else if (strcmp(current_profile, "performance") == 0) adw_combo_row_set_selected(profile_row, 2);
    } else {
        adw_combo_row_set_selected(profile_row, 1);
    }

    g_signal_connect(profile_row, "notify::selected", G_CALLBACK(on_power_profile_selected), NULL);
    adw_preferences_group_add(power_group, GTK_WIDGET(profile_row));

    /* ── Battery Status ─────────────────────────────────────────────── */
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Battery Status");
    adw_preferences_page_add(page, group);

    const char *bat_path = caps->battery_path;

    if (!caps->has_battery || !bat_path) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No battery found");
        adw_action_row_set_subtitle(row, "This may be a desktop system");
        adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("dialog-information-symbolic"));
        adw_preferences_group_add(group, GTK_WIDGET(row));
        return GTK_WIDGET(page);
    }

    /* Capacity */
    g_autofree char *cap_path = g_strdup_printf("%s/capacity", bat_path);
    g_autofree char *capacity = read_sys_file(cap_path);

    AdwActionRow *cap_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cap_row), "Battery Level");
    g_autofree char *cap_label = g_strdup_printf("%s%%", capacity ? capacity : "?");
    adw_action_row_set_subtitle(cap_row, cap_label);

    int cap_pct = capacity ? atoi(capacity) : 0;
    const char *bat_icon = cap_pct > 80 ? "battery-full-symbolic"
                         : cap_pct > 50 ? "battery-good-symbolic"
                         : cap_pct > 20 ? "battery-low-symbolic"
                                        : "battery-caution-symbolic";
    adw_action_row_add_suffix(cap_row, gtk_image_new_from_icon_name(bat_icon));
    adw_preferences_group_add(group, GTK_WIDGET(cap_row));

    /* Status */
    g_autofree char *status_path = g_strdup_printf("%s/status", bat_path);
    g_autofree char *status = read_sys_file(status_path);

    AdwActionRow *status_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(status_row), "Status");
    adw_action_row_set_subtitle(status_row, status ? status : "Unknown");
    const char *status_icon = (status && strcmp(status, "Charging") == 0)
        ? "battery-good-charging-symbolic" : "battery-good-symbolic";
    adw_action_row_add_suffix(status_row, gtk_image_new_from_icon_name(status_icon));
    adw_preferences_group_add(group, GTK_WIDGET(status_row));

    /* Cycle count */
    g_autofree char *cycle_path = g_strdup_printf("%s/cycle_count", bat_path);
    g_autofree char *cycles = read_sys_file(cycle_path);

    if (cycles && strlen(cycles) > 0 && strcmp(cycles, "0") != 0) {
        AdwActionRow *cycle_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cycle_row), "Cycle Count");
        adw_action_row_set_subtitle(cycle_row, cycles);
        adw_preferences_group_add(group, GTK_WIDGET(cycle_row));
    }

    /* Energy / full */
    g_autofree char *energy_path = g_strdup_printf("%s/energy_now", bat_path);
    g_autofree char *full_path = g_strdup_printf("%s/energy_full", bat_path);
    g_autofree char *energy_now = read_sys_file(energy_path);
    g_autofree char *energy_full = read_sys_file(full_path);

    if (energy_now && energy_full) {
        double now_wh = atof(energy_now) / 1000000.0;
        double full_wh = atof(energy_full) / 1000000.0;
        AdwPreferencesGroup *wh_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(wh_group, "Energy");
        adw_preferences_page_add(page, wh_group);

        AdwActionRow *wh_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(wh_row), "Remaining / Capacity");
        g_autofree char *wh_label = g_strdup_printf("%.1f Wh / %.1f Wh", now_wh, full_wh);
        adw_action_row_set_subtitle(wh_row, wh_label);
        adw_preferences_group_add(wh_group, GTK_WIDGET(wh_row));
    }

    return GTK_WIDGET(page);
}
