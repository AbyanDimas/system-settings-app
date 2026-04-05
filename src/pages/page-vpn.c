#include "page-vpn.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_wg_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    const char *iface = user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        g_autofree char *cmd = g_strdup_printf("pkexec wg-quick up %s", iface);
        run_pkexec_async(cmd);
    } else {
        g_autofree char *cmd = g_strdup_printf("pkexec wg-quick down %s", iface);
        run_pkexec_async(cmd);
    }
}

static void on_ovpn_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    const char *path = user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        g_autofree char *cmd = g_strdup_printf("pkexec openvpn --config \"%s\" --daemon", path);
        run_pkexec_async(cmd);
    } else {
        run_pkexec_async("pkexec pkill openvpn");
    }
}

static void on_ts_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec tailscale up");
    } else {
        run_pkexec_async("pkexec tailscale down");
    }
}

static void on_wg_gen(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("bash -c 'mkdir -p ~/wireguard && cd ~/wireguard && wg genkey | tee privatekey | wg pubkey > publickey'", &err);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Keypair generated in ~/wireguard/"));
    }
}

static void on_file_opened(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    const char *type = user_data;
    g_autoptr(GError) err = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, res, &err);
    if (file) {
        g_autofree char *path = g_file_get_path(file);
        g_autofree char *cmd = NULL;
        if (strcmp(type, "wg") == 0) {
            cmd = g_strdup_printf("pkexec cp \"%s\" /etc/wireguard/", path);
        } else {
            cmd = g_strdup_printf("pkexec cp \"%s\" /etc/openvpn/", path);
        }
        subprocess_run_async(cmd, NULL, NULL);
        g_object_unref(file);
        
        if (page_widget) {
            GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
            if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Configuration imported. Please restart settings app."));
        }
    }
}

static void on_import_wg(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Import WireGuard Config");
    if (page_widget) {
        GtkRoot *root = gtk_widget_get_root(page_widget);
        gtk_file_dialog_open(dialog, GTK_WINDOW(root), NULL, on_file_opened, "wg");
    }
}

static void on_import_ovpn(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Import OpenVPN Config");
    if (page_widget) {
        GtkRoot *root = gtk_widget_get_root(page_widget);
        gtk_file_dialog_open(dialog, GTK_WINDOW(root), NULL, on_file_opened, "ovpn");
    }
}

GtkWidget *page_vpn_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "VPN Manager");
    adw_preferences_page_set_icon_name(page, "network-vpn-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── WireGuard ─────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *wg_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(wg_group, "WireGuard Tunnels");
    adw_preferences_page_add(page, wg_group);

    AdwActionRow *wg_hdr = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(wg_hdr), "Manage WireGuard");
    GtkWidget *wg_bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *wg_b1 = gtk_button_new_with_label("Gen Keypair");
    gtk_widget_set_valign(wg_b1, GTK_ALIGN_CENTER);
    g_signal_connect(wg_b1, "clicked", G_CALLBACK(on_wg_gen), NULL);
    gtk_box_append(GTK_BOX(wg_bb), wg_b1);
    GtkWidget *wg_b2 = gtk_button_new_with_label("Import .conf");
    gtk_widget_add_css_class(wg_b2, "suggested-action");
    gtk_widget_set_valign(wg_b2, GTK_ALIGN_CENTER);
    g_signal_connect(wg_b2, "clicked", G_CALLBACK(on_import_wg), NULL);
    gtk_box_append(GTK_BOX(wg_bb), wg_b2);
    adw_action_row_add_suffix(wg_hdr, wg_bb);
    adw_preferences_group_add(wg_group, GTK_WIDGET(wg_hdr));

    g_autofree char *wg_out = subprocess_run_sync("sh -c 'ls -1 /etc/wireguard/*.conf ~/wireguard/*.conf 2>/dev/null'", &err);
    if (wg_out) {
        g_auto(GStrv) lines = g_strsplit(wg_out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) > 0) {
                char *slash = strrchr(line, '/');
                char *filename = slash ? slash + 1 : line;
                char *cpy = g_strdup(filename);
                char *dot = strrchr(cpy, '.');
                if (dot) *dot = '\0'; // Interface name e.g. wg0
                
                AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), cpy);
                adw_expander_row_set_subtitle(exp, line);

                AdwSwitchRow *sw = ADW_SWITCH_ROW(adw_switch_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sw), "Connection Active");
                
                g_autofree char *ck = g_strdup_printf("ip link show %s 2>/dev/null", cpy);
                g_autofree char *ipo = subprocess_run_sync(ck, &err);
                adw_switch_row_set_active(sw, (ipo && strlen(ipo) > 5));
                
                g_signal_connect_data(sw, "notify::active", G_CALLBACK(on_wg_toggled), g_strdup(cpy), free_g_free, 0);
                adw_expander_row_add_row(exp, GTK_WIDGET(sw));

                adw_preferences_group_add(wg_group, GTK_WIDGET(exp));
                g_free(cpy);
            }
            g_free(line);
        }
    }

    /* ── OpenVPN ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ov_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(ov_group, "OpenVPN Connections");
    adw_preferences_page_add(page, ov_group);

    AdwActionRow *ov_hdr = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ov_hdr), "Manage OpenVPN");
    GtkWidget *ov_b = gtk_button_new_with_label("Import .ovpn");
    gtk_widget_add_css_class(ov_b, "suggested-action");
    gtk_widget_set_valign(ov_b, GTK_ALIGN_CENTER);
    g_signal_connect(ov_b, "clicked", G_CALLBACK(on_import_ovpn), NULL);
    adw_action_row_add_suffix(ov_hdr, ov_b);
    adw_preferences_group_add(ov_group, GTK_WIDGET(ov_hdr));

    g_autofree char *ov_out = subprocess_run_sync("sh -c 'ls -1 /etc/openvpn/*.ovpn ~/.config/openvpn/*.ovpn ~/openvpn/*.ovpn 2>/dev/null'", &err);
    if (ov_out) {
        g_auto(GStrv) lines = g_strsplit(ov_out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) > 0) {
                char *slash = strrchr(line, '/');
                char *filename = slash ? slash + 1 : line;

                AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), filename);
                adw_expander_row_set_subtitle(exp, line);

                AdwSwitchRow *sw = ADW_SWITCH_ROW(adw_switch_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sw), "Connect Daemon");
                
                g_autofree char *ck = subprocess_run_sync("pgrep openvpn", &err);
                adw_switch_row_set_active(sw, (ck && strlen(ck) > 0)); 
                // Simplified: if any openvpn running we show active
                
                g_signal_connect_data(sw, "notify::active", G_CALLBACK(on_ovpn_toggled), g_strdup(line), free_g_free, 0);
                adw_expander_row_add_row(exp, GTK_WIDGET(sw));

                adw_preferences_group_add(ov_group, GTK_WIDGET(exp));
            }
            g_free(line);
        }
    }

    /* ── Tailscale ─────────────────────────────────────────────────────────── */
    if (g_find_program_in_path("tailscale")) {
        AdwPreferencesGroup *ts_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(ts_group, "Tailscale Mesh");
        adw_preferences_page_add(page, ts_group);

        AdwSwitchRow *sw = ADW_SWITCH_ROW(adw_switch_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sw), "Tailscale Active");
        
        g_autofree char *ts_out = subprocess_run_sync("tailscale status", &err);
        adw_switch_row_set_active(sw, (ts_out && !strstr(ts_out, "Tailscale is stopped") && !strstr(ts_out, "Logged out")));
        
        g_signal_connect(sw, "notify::active", G_CALLBACK(on_ts_toggled), NULL);
        adw_preferences_group_add(ts_group, GTK_WIDGET(sw));
        
        if (ts_out) {
            AdwActionRow *sr = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr), "Status");
            char *line1 = g_strdup(ts_out);
            char *nl = strchr(line1, '\n');
            if (nl) *nl = '\0';
            adw_action_row_set_subtitle(sr, line1);
            adw_preferences_group_add(ts_group, GTK_WIDGET(sr));
            g_free(line1);
        }
    }

    return overlay;
}
