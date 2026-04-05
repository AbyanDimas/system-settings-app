#include "page-intrusion.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *jails_group = NULL;
static GtkWidget *jails_list_box = NULL;
static AdwSwitchRow *r_enable = NULL;

static void free_g_free(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void refresh_fail2ban(void);

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec systemctl enable --now fail2ban");
    } else {
        run_pkexec_async("pkexec systemctl disable --now fail2ban");
    }
}

static void on_unban_ip(GtkButton *btn, gpointer user_data) {
    (void)btn;
    char **args = user_data; // array [jail, ip]
    if (args && args[0] && args[1]) {
        g_autofree char *cmd = g_strdup_printf("pkexec fail2ban-client set %s unbanip %s", args[0], args[1]);
        g_autoptr(GError) err = NULL;
        subprocess_run_sync(cmd, &err);
        refresh_fail2ban();
    }
}

static void free_unban_args(gpointer data, GClosure *closure) {
    (void)closure;
    g_strfreev((char**)data);
}

static void refresh_fail2ban(void) {
    if (!jails_list_box) return;
    
    gtk_list_box_remove_all(GTK_LIST_BOX(jails_list_box));

    g_autoptr(GError) err = NULL;
    
    g_autofree char *sys_out = subprocess_run_sync("systemctl is-active fail2ban", &err);
    gboolean is_active = (sys_out && strstr(sys_out, "active") && !strstr(sys_out, "inactive"));
    
    g_signal_handlers_block_by_func(r_enable, on_enable_toggled, NULL);
    adw_switch_row_set_active(r_enable, is_active);
    g_signal_handlers_unblock_by_func(r_enable, on_enable_toggled, NULL);

    if (!is_active) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Fail2ban is currently inactive.");
        gtk_list_box_append(GTK_LIST_BOX(jails_list_box), GTK_WIDGET(row));
        return;
    }

    g_autofree char *out = subprocess_run_sync("pkexec fail2ban-client status", &err);
    if (!out) return;

    char *jail_line = strstr(out, "Jail list:");
    if (!jail_line) return;

    jail_line += 10;
    while (*jail_line == ' ' || *jail_line == '\t') jail_line++;

    g_auto(GStrv) jails = g_strsplit(jail_line, ",", -1);
    for (int i = 0; jails[i] != NULL; i++) {
        char *jail = g_strstrip(g_strdup(jails[i]));
        if (strlen(jail) == 0) { g_free(jail); continue; }

        AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), g_strdup_printf("Jail: %s", jail));

        g_autofree char *cmd = g_strdup_printf("pkexec fail2ban-client status %s", jail);
        g_autofree char *j_out = subprocess_run_sync(cmd, &err);

        if (j_out) {
            char *banned_line = strstr(j_out, "Banned IP list:");
            if (banned_line) {
                banned_line += 15;
                char *endln = strchr(banned_line, '\n');
                if (endln) *endln = '\0';
                
                g_strstrip(banned_line);
                if (strlen(banned_line) > 0) {
                    adw_expander_row_set_subtitle(exp, "Contains banned IPs");
                    
                    g_auto(GStrv) ips = g_strsplit(banned_line, " ", -1);
                    for (int k = 0; ips[k] != NULL; k++) {
                        char *ip = g_strstrip(g_strdup(ips[k]));
                        if (strlen(ip) == 0) { g_free(ip); continue; }

                        AdwActionRow *ip_row = ADW_ACTION_ROW(adw_action_row_new());
                        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ip_row), ip);
                        
                        GtkWidget *un_btn = gtk_button_new_with_label("Unban");
                        gtk_widget_add_css_class(un_btn, "destructive-action");
                        gtk_widget_set_valign(un_btn, GTK_ALIGN_CENTER);

                        char **args = g_new0(char*, 3);
                        args[0] = g_strdup(jail);
                        args[1] = g_strdup(ip);
                        
                        g_signal_connect_data(un_btn, "clicked", G_CALLBACK(on_unban_ip), args, free_unban_args, 0);
                        adw_action_row_add_suffix(ip_row, un_btn);
                        
                        adw_expander_row_add_row(exp, GTK_WIDGET(ip_row));
                        g_free(ip);
                    }
                } else {
                    adw_expander_row_set_subtitle(exp, "0 IPs banned currently");
                    AdwActionRow *nr = ADW_ACTION_ROW(adw_action_row_new());
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nr), "No active bans");
                    adw_expander_row_add_row(exp, GTK_WIDGET(nr));
                }
            }
        }
        
        gtk_list_box_append(GTK_LIST_BOX(jails_list_box), GTK_WIDGET(exp));
        g_free(jail);
    }
}

static gboolean on_auto_refresh(gpointer user_data) {
    (void)user_data;
    refresh_fail2ban();
    return G_SOURCE_CONTINUE;
}

GtkWidget *page_intrusion_new(void) {
    if (!g_find_program_in_path("fail2ban-client")) {
        return omarchy_make_unavailable_page("Intrusion Detection", "sudo pacman -S fail2ban", "network-wireless-acquiring-symbolic"); // or some shield
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Intrusion Detection");
    adw_preferences_page_set_icon_name(page, "security-high-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "Service Status");
    adw_preferences_page_add(page, st_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Enable Fail2Ban Filter");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_enable), "Runs systemd service for IP banning");
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_enable));

    AdwActionRow *ref_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ref_row), "Refresh Jails List");
    GtkWidget *ref_btn = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_set_valign(ref_btn, GTK_ALIGN_CENTER);
    g_signal_connect(ref_btn, "clicked", G_CALLBACK(refresh_fail2ban), NULL);
    adw_action_row_add_suffix(ref_row, ref_btn);
    adw_preferences_group_add(st_group, GTK_WIDGET(ref_row));

    jails_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(jails_group, "Active Jails");
    adw_preferences_group_set_description(jails_group, "Jails are rule groups that scan logs for malicious activity.");
    
    jails_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(jails_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(jails_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(jails_group, jails_list_box);
    adw_preferences_page_add(page, jails_group);

    refresh_fail2ban();
    g_timeout_add_seconds(30, on_auto_refresh, NULL);

    return overlay;
}
