#include "page-bluetooth.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>

/* ── Bluetooth page ────────────────────────────────────────────────────────
 * Bluetooth on/off via rfkill + list of paired devices from bluetoothctl.
 * ─────────────────────────────────────────────────────────────────────────*/

static GtkWidget *bt_list_box = NULL;
static GtkWidget *page_widget = NULL;
static guint timer_id = 0;

static void on_device_action(AdwActionRow *row, gpointer user_data) {
    (void)user_data;
    const char *mac = adw_action_row_get_subtitle(row);
    // Mac might have (Connected) suffix in our new subtitle logic, let's strip it
    g_autofree char *clean_mac = g_strdup(mac);
    char *paren = strchr(clean_mac, ' ');
    if (paren) *paren = '\0';

    const char *title = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));

    g_autofree char *info_cmd = g_strdup_printf("bluetoothctl info %s", clean_mac);
    g_autofree char *info_out = subprocess_run_sync(info_cmd, NULL);
    gboolean currently_connected = (info_out && strstr(info_out, "Connected: yes"));

    g_autofree char *cmd = NULL;
    if (currently_connected) {
        cmd = g_strdup_printf("bluetoothctl disconnect %s", clean_mac);
    } else {
        cmd = g_strdup_printf("bluetoothctl connect %s", clean_mac);
    }

    g_autoptr(GError) err = NULL;
    subprocess_run_async(cmd, NULL, NULL);
    
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            g_autofree char *msg = g_strdup_printf("%s %s...", currently_connected ? "Disconnecting from" : "Connecting to", title);
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
}

static void on_bluetooth_toggle(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    if (active) {
        subprocess_run_sync("rfkill unblock bluetooth", &err);
        subprocess_run_sync("bluetoothctl power on", &err);
    } else {
        subprocess_run_sync("bluetoothctl power off", &err);
        subprocess_run_sync("rfkill block bluetooth", &err);
    }
}

static void on_bt_scan_complete(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkWidget *list_box = GTK_WIDGET(user_data);
    if (!source_object) return;
    GSubprocess *proc = G_SUBPROCESS(source_object);
    g_autoptr(GError) error = NULL;
    g_autofree char *stdout_buf = NULL;

    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, NULL, &error);
    if (!stdout_buf || error) return;

    /* Output: "Device XX:XX:XX:XX:XX:XX Device Name" */
    g_auto(GStrv) lines = g_strsplit(stdout_buf, "\n", -1);
    gboolean any = FALSE;
    for (int i = 0; lines[i] != NULL; i++) {
        if (!g_str_has_prefix(lines[i], "Device ")) continue;
        /* Split "Device MAC Name" */
        g_auto(GStrv) parts = g_strsplit(lines[i], " ", 3);
        if (!parts[0] || !parts[1] || !parts[2]) continue;

        const char *mac = parts[1];
        const char *name = parts[2];
        if (strlen(name) == 0) name = mac;

        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), name);
        adw_action_row_set_subtitle(row, mac);
        
        g_autofree char *info_cmd = g_strdup_printf("bluetoothctl info %s", mac);
        g_autofree char *info_out = subprocess_run_sync(info_cmd, NULL);
        if (info_out && strstr(info_out, "Connected: yes")) {
            adw_action_row_set_subtitle(row, g_strdup_printf("%s (Connected)", mac));
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("emblem-ok-symbolic"));
        } else {
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("bluetooth-active-symbolic"));
        }
        
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_signal_connect(row, "activated", G_CALLBACK(on_device_action), NULL);
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
        any = TRUE;
    }

    if (!any) {
        AdwActionRow *empty = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(empty), "No paired devices");
        adw_action_row_set_subtitle(empty, "Use bluetoothctl to pair devices");
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(empty));
    }
}

static gboolean refresh_bluetooth_devices(gpointer user_data) {
    (void)user_data;
    if (!bt_list_box) return G_SOURCE_REMOVE;
    gtk_list_box_remove_all(GTK_LIST_BOX(bt_list_box));
    subprocess_run_async("bluetoothctl devices", on_bt_scan_complete, bt_list_box);
    return G_SOURCE_CONTINUE;
}

static void on_page_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    if (timer_id > 0) {
        g_source_remove(timer_id);
        timer_id = 0;
    }
    bt_list_box = NULL;
}

GtkWidget *page_bluetooth_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (!caps->has_bluetoothctl) {
        return omarchy_make_unavailable_page("bluetoothctl", "sudo pacman -S bluez bluez-utils", "bluetooth-active-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Bluetooth");
    adw_preferences_page_set_icon_name(page, "bluetooth-active-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));
    g_signal_connect(overlay, "destroy", G_CALLBACK(on_page_destroy), NULL);

    /* ── Toggle ──────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    /* Check current state via rfkill */
    g_autoptr(GError) err = NULL;
    g_autofree char *rfkill_out = subprocess_run_sync("rfkill list bluetooth", &err);
    /* If output contains "Soft blocked: no" bluetooth is on */
    gboolean bt_on = rfkill_out &&
        (strstr(rfkill_out, "Soft blocked: no") != NULL ||
         strstr(rfkill_out, "blocked: no") != NULL);

    AdwSwitchRow *bt_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bt_row), "Bluetooth");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(bt_row), "Enable or disable Bluetooth");
    adw_switch_row_set_active(bt_row, bt_on);
    g_signal_connect(bt_row, "notify::active", G_CALLBACK(on_bluetooth_toggle), NULL);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(bt_row));

    /* ── Paired devices ─────────────────────────────────────────────── */
    AdwPreferencesGroup *dev_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(dev_group, "Paired Devices");
    adw_preferences_page_add(page, dev_group);

    bt_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(bt_list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(bt_list_box, "boxed-list");
    adw_preferences_group_add(dev_group, bt_list_box);

    refresh_bluetooth_devices(NULL);
    timer_id = g_timeout_add_seconds(5, refresh_bluetooth_devices, NULL);

    return overlay;
}
