#include "page-netmonitor.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *live_group = NULL;
static guint timer_id = 0;

typedef struct {
    char name[64];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    AdwActionRow *row;
} NetInterface;

static NetInterface ifaces[32];
static int iface_count = 0;

static unsigned long long read_sys_val(const char *iface, const char *file) {
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/%s", iface, file);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    unsigned long long val = 0;
    if (fscanf(f, "%llu", &val) != 1) val = 0;
    fclose(f);
    return val;
}

static char *format_bytes(double bytes) {
    if (bytes > 1073741824.0) return g_strdup_printf("%.2f GB/s", bytes / 1073741824.0);
    if (bytes > 1048576.0) return g_strdup_printf("%.2f MB/s", bytes / 1048576.0);
    if (bytes > 1024.0) return g_strdup_printf("%.2f KB/s", bytes / 1024.0);
    return g_strdup_printf("%.0f B/s", bytes);
}

static char *format_total(unsigned long long bytes) {
    double b = (double)bytes;
    if (b > 1073741824.0) return g_strdup_printf("%.2f GB", b / 1073741824.0);
    if (b > 1048576.0) return g_strdup_printf("%.2f MB", b / 1048576.0);
    if (b > 1024.0) return g_strdup_printf("%.2f KB", b / 1024.0);
    return g_strdup_printf("%.0f B", b);
}

static gboolean on_refresh_net(gpointer data) {
    (void)data;
    if (!page_widget || !live_group) return G_SOURCE_REMOVE;

    DIR *d = opendir("/sys/class/net");
    if (!d) return G_SOURCE_CONTINUE;
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        if (strcmp(dir->d_name, "lo") == 0) continue;

        unsigned long long cur_rx = read_sys_val(dir->d_name, "rx_bytes");
        unsigned long long cur_tx = read_sys_val(dir->d_name, "tx_bytes");

        NetInterface *fi = NULL;
        for (int i = 0; i < iface_count; i++) {
            if (strcmp(ifaces[i].name, dir->d_name) == 0) {
                fi = &ifaces[i];
                break;
            }
        }

        if (!fi) {
            if (iface_count >= 32) break;
            fi = &ifaces[iface_count++];
            strncpy(fi->name, dir->d_name, 63);
            fi->rx_bytes = cur_rx;
            fi->tx_bytes = cur_tx;
            
            fi->row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(fi->row), dir->d_name);
            adw_preferences_group_add(live_group, GTK_WIDGET(fi->row));
            continue; // first tick has no speed
        }

        // calculate diff
        double rx_speed = (double)(cur_rx - fi->rx_bytes);
        double tx_speed = (double)(cur_tx - fi->tx_bytes);

        fi->rx_bytes = cur_rx;
        fi->tx_bytes = cur_tx;

        g_autofree char *rs = format_bytes(rx_speed);
        g_autofree char *ts = format_bytes(tx_speed);
        g_autofree char *rtot = format_total(cur_rx);
        g_autofree char *ttot = format_total(cur_tx);

        g_autofree char *sub = g_strdup_printf("↓ %s   ↑ %s    (Total: ↓ %s  ↑ %s)", rs, ts, rtot, ttot);
        adw_action_row_set_subtitle(fi->row, sub);
    }
    closedir(d);

    return G_SOURCE_CONTINUE;
}

static void on_page_destroy(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (timer_id > 0) {
        g_source_remove(timer_id);
        timer_id = 0;
    }
    page_widget = NULL;
}

GtkWidget *page_netmonitor_new(void) {
    iface_count = 0;

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    g_signal_connect(overlay, "destroy", G_CALLBACK(on_page_destroy), NULL);

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Network Monitor");
    adw_preferences_page_set_icon_name(page, "network-transmit-receive-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Live Interface Speeds ─────────────────────────────────────────────── */
    live_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(live_group, "Live Traffic");
    adw_preferences_group_set_description(live_group, "Real-time bandwidth consumption per interface");
    adw_preferences_page_add(page, live_group);

    on_refresh_net(NULL);
    timer_id = g_timeout_add(1000, on_refresh_net, NULL);

    /* ── Historical Logs ───────────────────────────────────────────────────── */
    AdwPreferencesGroup *hist_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(hist_group, "Historical Usage (vnstat)");
    adw_preferences_page_add(page, hist_group);

    if (g_find_program_in_path("vnstat")) {
        GtkWidget *scroll = gtk_scrolled_window_new();
        gtk_widget_set_size_request(scroll, -1, 400);
        
        GtkWidget *tv = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), 6);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 6);
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 6);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 6);
        
        g_autoptr(GError) err = NULL;
        g_autofree char *out = subprocess_run_sync("vnstat", &err);
        if (out) {
            GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
            gtk_text_buffer_set_text(buf, out, -1);
        }
        
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
        adw_preferences_group_add(hist_group, scroll);
    } else {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "vnstat is not installed.");
        adw_action_row_set_subtitle(row, "Install vnstat to track daily/monthly data usage caps.");
        adw_preferences_group_add(hist_group, GTK_WIDGET(row));
    }

    return overlay;
}
