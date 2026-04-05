#include "page-sysinfo.h"
#include <adwaita.h>
#include "../utils/tool-detector.h"

static void add_info_row(AdwPreferencesGroup *group, const char *title, const char *subtitle, const char *icon) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    adw_action_row_set_subtitle(row, subtitle);
    if (icon) {
        GtkWidget *img = gtk_image_new_from_icon_name(icon);
        adw_action_row_add_prefix(row, img);
    }
    adw_preferences_group_add(group, GTK_WIDGET(row));
}

GtkWidget *page_sysinfo_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Info");
    adw_preferences_page_set_icon_name(page, "emblem-system-symbolic");

    OmarchySystemCaps *caps = omarchy_get_system_caps();

    /* Compositor */
    AdwPreferencesGroup *g_comp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_comp, "Compositor");
    add_info_row(g_comp, "Display Server", caps->compositor, "video-display-symbolic");
    adw_preferences_page_add(page, g_comp);

    /* Network */
    AdwPreferencesGroup *g_net = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_net, "Network");
    add_info_row(g_net, "Backend", caps->network_backend, "network-wireless-symbolic");
    g_autofree char *net_tools = g_strdup_printf("NMCLI: %s | IWD: %s | WPA: %s", 
      caps->has_nmcli ? "✓" : "✗", caps->has_iwd ? "✓" : "✗", caps->has_wpa_cli ? "✓" : "✗");
    add_info_row(g_net, "Tools", net_tools, "utilities-terminal-symbolic");
    adw_preferences_page_add(page, g_net);

    /* Audio */
    AdwPreferencesGroup *g_aud = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_aud, "Audio");
    add_info_row(g_aud, "Backend", caps->audio_backend, "audio-volume-high-symbolic");
    g_autofree char *aud_tools = g_strdup_printf("PipeWire: %s | PulseAudio: %s", 
      caps->has_pipewire ? "✓" : "✗", caps->has_pulseaudio ? "✓" : "✗");
    add_info_row(g_aud, "Daemons", aud_tools, "utilities-terminal-symbolic");
    adw_preferences_page_add(page, g_aud);

    /* Notifications */
    AdwPreferencesGroup *g_not = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_not, "Notifications");
    add_info_row(g_not, "Daemon", caps->notification_daemon, "preferences-system-notifications-symbolic");
    adw_preferences_page_add(page, g_not);

    /* Hardware */
    AdwPreferencesGroup *g_hw = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_hw, "Hardware Features");
    add_info_row(g_hw, "Battery", caps->has_battery ? caps->battery_path : "Not present", "battery-good-symbolic");
    add_info_row(g_hw, "Backlight", caps->has_backlight ? caps->backlight_path : "Not present", "display-brightness-symbolic");
    add_info_row(g_hw, "Bluetooth", caps->has_bluetoothctl ? "Available" : "Not available", "bluetooth-active-symbolic");
    adw_preferences_page_add(page, g_hw);

    /* Other */
    AdwPreferencesGroup *g_oth = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(g_oth, "Other Facilities");
    add_info_row(g_oth, "Wallpaper Tool", caps->wallpaper_tool, "preferences-desktop-wallpaper-symbolic");
    add_info_row(g_oth, "Display Tool", caps->display_tool, "video-display-symbolic");
    add_info_row(g_oth, "Firewall", caps->firewall_tool, "security-high-symbolic");
    add_info_row(g_oth, "Power Profile", caps->power_tool, "preferences-system-power-symbolic");
    adw_preferences_page_add(page, g_oth);

    return GTK_WIDGET(page);
}
