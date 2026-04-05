#include "page-network.h"
#include "../window.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>
#include <string.h>

typedef struct {
    GtkWidget *container;
    char *backend;
} ScanData;

static GtkWidget *net_list_box = NULL;
static GtkWidget *page_widget = NULL;
static guint timer_id = 0;

static char *get_wifi_interface(OmarchySystemCaps *caps);

typedef struct {
    char *ssid;
    char *iface;
    char *backend;
    AdwDialog *dialog;
    AdwEntryRow *password_row;
} ConnectionData;

static void on_connect_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    ConnectionData *data = user_data;
    const char *password = gtk_editable_get_text(GTK_EDITABLE(data->password_row));
    g_autofree char *cmd = NULL;

    if (strcmp(data->backend, "iwd") == 0) {
        cmd = g_strdup_printf("iwctl station %s connect %s --passphrase %s", data->iface, data->ssid, password);
    } else {
        cmd = g_strdup_printf("nmcli dev wifi connect %s password %s", data->ssid, password);
    }

    g_autoptr(GError) err = NULL;
    subprocess_run_async(cmd, NULL, NULL);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Connecting..."));
    }
    adw_dialog_close(data->dialog);
}

static void free_conn_data(gpointer data, GClosure *closure) {
    (void)closure;
    ConnectionData *cd = data;
    g_free(cd->ssid);
    g_free(cd->iface);
    g_free(cd->backend);
    g_free(cd);
}

static void on_network_activated(AdwActionRow *row, gpointer user_data) {
    (void)user_data;
    const char *ssid = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    const char *sub = adw_action_row_get_subtitle(row);
    if (sub && strcmp(sub, "Connected") == 0) return;

    OmarchySystemCaps *caps = omarchy_get_system_caps();
    g_autofree char *iface = get_wifi_interface(caps);

    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Connect to Wi-Fi");
    adw_dialog_set_content_width(dialog, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);

    GtkWidget *lbl = gtk_label_new(NULL);
    g_autofree char *markup = g_strdup_printf("Enter password for <b>%s</b>", ssid);
    gtk_label_set_markup(GTK_LABEL(lbl), markup);
    gtk_box_append(GTK_BOX(vbox), lbl);

    AdwEntryRow *pw_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pw_row), "Password");
    // In GTK4 AdwEntryRow, the child is the GtkText or GtkEntry
    GtkWidget *entry = gtk_widget_get_first_child(GTK_WIDGET(pw_row));
    while (entry && !GTK_IS_EDITABLE(entry)) entry = gtk_widget_get_next_sibling(entry);
    if (entry) gtk_text_set_visibility(GTK_TEXT(entry), FALSE);
    
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(pw_row));

    ConnectionData *cdata = g_new0(ConnectionData, 1);
    cdata->ssid = g_strdup(ssid);
    cdata->iface = g_strdup(iface);
    cdata->backend = g_strdup(caps->network_backend);
    cdata->dialog = dialog;
    cdata->password_row = pw_row;

    GtkWidget *btn = gtk_button_new_with_label("Connect");
    gtk_widget_add_css_class(btn, "suggested-action");
    g_signal_connect_data(btn, "clicked", G_CALLBACK(on_connect_clicked), cdata, (GClosureNotify)free_conn_data, 0);
    gtk_box_append(GTK_BOX(vbox), btn);

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, page_widget);
}

static void strip_ansi_escapes(char *str) {
    if (!str) return;
    char *read = str;
    char *write = str;
    while (*read) {
        if (*read == '\x1B' && *(read + 1) == '[') {
            read += 2;
            while (*read && *read != 'm' && *read != 'K' && *read != 'G') read++;
            if (*read) read++; 
            continue;
        }
        *write++ = *read++;
    }
    *write = '\0';
}

static char *get_wifi_interface(OmarchySystemCaps *caps) {
    g_autoptr(GError) err = NULL;
    char *out = NULL;
    if (strcmp(caps->network_backend, "networkmanager") == 0) {
        out = subprocess_run_sync("sh -c \"nmcli -t -f DEVICE,TYPE dev | grep wifi | head -n1 | cut -d: -f1\"", &err);
    } else if (strcmp(caps->network_backend, "iwd") == 0) {
        out = subprocess_run_sync("sh -c \"iwctl device list | grep -o 'wlan[0-9]' | head -n1\"", &err);
    } else if (strcmp(caps->network_backend, "wpa_supplicant") == 0) {
        out = subprocess_run_sync("sh -c \"ip link | grep -o 'wlan[0-9]' | head -n1\"", &err);
    }
    
    if (out && strlen(out) > 1) {
        char *iface = g_strstrip(g_strdup(out));
        g_free(out);
        return iface;
    }
    return g_strdup("wlan0");
}

static void parse_iwctl_networks(const char *output, GtkWidget *list_box) {
    if (!output) return;
    g_auto(GStrv) lines = g_strsplit(output, "\n", -1);
    gboolean data_started = FALSE;
    for (int i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "--")) { data_started = TRUE; continue; }
        if (!data_started) continue;
        if (strstr(lines[i], "Network name") && strstr(lines[i], "Security")) continue;
        if (strlen(lines[i]) < 36) continue;
        
        gboolean in_use = (lines[i][0] == '>');
        g_autofree char *ssid = g_strstrip(g_strndup(lines[i] + 4, 32));
        if (strlen(ssid) == 0) continue;
        
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), ssid);
        
        GtkWidget *icon;
        if (in_use) {
            adw_action_row_set_subtitle(row, "Connected");
            icon = gtk_image_new_from_icon_name("emblem-ok-symbolic");
        } else {
            icon = gtk_image_new_from_icon_name("network-wireless-symbolic");
        }
        adw_action_row_add_suffix(row, icon);
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_signal_connect(row, "activated", G_CALLBACK(on_network_activated), NULL);
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
    }
}

static void parse_nmcli_networks(const char *output, GtkWidget *list_box) {
    if (!output) return;
    g_auto(GStrv) lines = g_strsplit(output, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (strlen(lines[i]) == 0) continue;
        g_auto(GStrv) parts = g_strsplit(lines[i], ":", 4);
        // nmcli output needs at least 4 fields. If colons appear in SSID, it breaks simple split.
        // We will do a basic check.
        if (!parts[0] || !parts[1] || !parts[2]) continue;
        
        char *ssid = parts[0];
        char *signal = parts[1];
        char *sec = parts[2];
        char *in_use = parts[3] ? parts[3] : "";
        if (strlen(ssid) == 0) continue;
        
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), ssid);
        
        if (strcmp(in_use, "*") == 0) {
            adw_action_row_set_subtitle(row, "Connected");
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("emblem-ok-symbolic"));
        } else {
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("network-wireless-symbolic"));
        }
        
        g_autofree char *details = g_strdup_printf("%s%% | %s", signal, sec);
        GtkWidget *label = gtk_label_new(details);
        gtk_widget_add_css_class(label, "dim-label");
        adw_action_row_add_suffix(row, label);
        
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_signal_connect(row, "activated", G_CALLBACK(on_network_activated), NULL);
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
    }
}

static void on_wifi_scan_complete(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    ScanData *data = (ScanData *)user_data;
    if (!source_object) { g_free(data->backend); g_free(data); return; }
    GtkWidget *container = data->container;
    GSubprocess *proc = G_SUBPROCESS(source_object);
    g_autoptr(GError) error = NULL;
    g_autofree char *stdout_buf = NULL;

    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, NULL, &error);
    if (stdout_buf && !error) {
        if (strcmp(data->backend, "networkmanager") == 0) {
            parse_nmcli_networks(stdout_buf, container);
        } else if (strcmp(data->backend, "iwd") == 0) {
            strip_ansi_escapes(stdout_buf);
            parse_iwctl_networks(stdout_buf, container);
        }
    }
    g_free(data->backend);
    g_free(data);
}

static gboolean refresh_networks(gpointer user_data) {
    (void)user_data;
    if (!net_list_box) return G_SOURCE_REMOVE;

    gtk_list_box_remove_all(GTK_LIST_BOX(net_list_box));

    OmarchySystemCaps *caps = omarchy_get_system_caps();
    g_autofree char *iface = get_wifi_interface(caps);

    ScanData *data = g_new0(ScanData, 1);
    data->container = net_list_box;
    data->backend = g_strdup(caps->network_backend);

    if (strcmp(caps->network_backend, "networkmanager") == 0) {
        subprocess_run_async("nmcli -t -f SSID,SIGNAL,SECURITY,IN-USE dev wifi list", on_wifi_scan_complete, data);
    } else if (strcmp(caps->network_backend, "iwd") == 0) {
        g_autofree char *cmd = g_strdup_printf("iwctl station %s get-networks", iface);
        subprocess_run_async(cmd, on_wifi_scan_complete, data);
    }
    return G_SOURCE_CONTINUE;
}

static void on_page_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    if (timer_id > 0) {
        g_source_remove(timer_id);
        timer_id = 0;
    }
    net_list_box = NULL;
}

static void on_wifi_toggle(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    g_autofree char *iface = get_wifi_interface(caps);

    if (active) {
        subprocess_run_sync("rfkill unblock wlan", NULL);
        if (strcmp(caps->network_backend, "iwd") == 0) {
            g_autofree char *cmd = g_strdup_printf("iwctl device %s set-property Powered on", iface);
            subprocess_run_sync(cmd, NULL);
        }
    } else {
        if (strcmp(caps->network_backend, "iwd") == 0) {
            g_autofree char *cmd = g_strdup_printf("iwctl device %s set-property Powered off", iface);
            subprocess_run_sync(cmd, NULL);
        }
        subprocess_run_sync("rfkill block wlan", NULL);
    }
}

static char *extract_iwctl_field(const char *output, const char *field_name) {
    if (!output) return NULL;
    g_autofree char *search = g_strdup_printf("  %s", field_name);
    const char *pos = strstr(output, search);
    if (!pos) return NULL;
    
    pos += strlen(search);
    while (*pos == ' ') pos++;
    
    const char *end = strchr(pos, '\n');
    if (!end) end = pos + strlen(pos);
    
    char *res = g_strndup(pos, end - pos);
    return g_strstrip(res);
}

GtkWidget *page_network_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->network_backend, "none") == 0) {
        return omarchy_make_unavailable_page("Network Daemon", "sudo pacman -S networkmanager", "network-wireless-symbolic");
    } else if (strcmp(caps->network_backend, "wpa_supplicant") == 0) {
        return omarchy_make_unavailable_page("wpa_supplicant GUI support", "sudo pacman -S networkmanager", "network-wireless-symbolic");
    }

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Network");
    adw_preferences_page_set_icon_name(page, "network-wireless-symbolic");
    g_signal_connect(page, "destroy", G_CALLBACK(on_page_destroy), NULL);

    g_autofree char *iface = get_wifi_interface(caps);

    /* ── WiFi on/off group ──────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    g_autofree char *rfkill_out = subprocess_run_sync("rfkill list wlan", NULL);
    gboolean wifi_on = TRUE;
    if (rfkill_out && strstr(rfkill_out, "Soft blocked: yes")) {
        wifi_on = FALSE;
    }

    AdwSwitchRow *wifi_row = ADW_SWITCH_ROW(adw_switch_row_new());
    g_autofree char *wifi_title = g_strdup_printf("Wi-Fi (%s)", iface);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(wifi_row), wifi_title);
    g_autofree char *wifi_sub = g_strdup_printf("Managed by %s", caps->network_backend);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(wifi_row), wifi_sub);
    adw_switch_row_set_active(wifi_row, wifi_on);
    g_signal_connect(wifi_row, "notify::active", G_CALLBACK(on_wifi_toggle), NULL);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(wifi_row));

    if (wifi_on) {
        /* Info Group */
        g_autofree char *conn_net = NULL;
        bool connected = false;

        if (strcmp(caps->network_backend, "networkmanager") == 0) {
            g_autofree char *nm_out = subprocess_run_sync("nmcli -t -f ACTIVE,SSID dev wifi | grep '^yes' | cut -d: -f2", NULL);
            if (nm_out && strlen(nm_out) > 1) {
                conn_net = g_strstrip(g_strdup(nm_out));
                connected = true;
            }
        } else if (strcmp(caps->network_backend, "iwd") == 0) {
            g_autofree char *cmd = g_strdup_printf("iwctl station %s show", iface);
            g_autofree char *station_out = subprocess_run_sync(cmd, NULL);
            strip_ansi_escapes(station_out);
            conn_net = extract_iwctl_field(station_out, "Connected network");
            if (conn_net && strlen(conn_net) > 0) connected = true;
        }

        if (connected) {
            AdwPreferencesGroup *info_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
            adw_preferences_group_set_title(info_group, "Current Connection");
            adw_preferences_page_add(page, info_group);
            
            AdwActionRow *con_row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(con_row), conn_net);
            adw_preferences_group_add(info_group, GTK_WIDGET(con_row));
        }

        /* ── Available networks group ────────────────────────────────────── */
        AdwPreferencesGroup *net_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(net_group, "Available Networks");
        adw_preferences_page_add(page, net_group);

        net_list_box = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(net_list_box), GTK_SELECTION_NONE);
        gtk_widget_add_css_class(net_list_box, "boxed-list");
        adw_preferences_group_add(net_group, net_list_box);

        refresh_networks(NULL);
        timer_id = g_timeout_add_seconds(8, refresh_networks, NULL);
    }

    return GTK_WIDGET(page);
}
