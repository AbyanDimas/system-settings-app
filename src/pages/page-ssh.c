#include "page-ssh.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwSwitchRow *r_enable = NULL;
static AdwSpinRow *r_port = NULL;
static AdwComboRow *r_root = NULL;
static AdwSwitchRow *r_pass = NULL;
static AdwSwitchRow *r_x11 = NULL;

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec systemctl enable --now sshd");
    } else {
        run_pkexec_async("pkexec systemctl disable --now sshd");
    }
}

static void on_apply_ssh(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    
    int port = (int)adw_spin_row_get_value(r_port);
    guint root_idx = adw_combo_row_get_selected(r_root);
    gboolean pass = adw_switch_row_get_active(r_pass);
    gboolean x11 = adw_switch_row_get_active(r_x11);

    const char *root_vals[] = {"yes", "prohibit-password", "no"};
    
    g_autofree char *cmd = g_strdup_printf(
        "pkexec bash -c '"
        "update_sshd() { "
        "  grep -q \"^#*\\s*$1\" /etc/ssh/sshd_config && "
        "  sed -i \"s/^#*\\s*$1.*/$1 $2/\" /etc/ssh/sshd_config || "
        "  echo \"$1 $2\" >> /etc/ssh/sshd_config; "
        "}; "
        "update_sshd Port %d; "
        "update_sshd PermitRootLogin %s; "
        "update_sshd PasswordAuthentication %s; "
        "update_sshd X11Forwarding %s; "
        "systemctl restart sshd'",
        port, root_vals[root_idx], pass ? "yes" : "no", x11 ? "yes" : "no"
    );

    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("SSH Configuration saved & restarted"));
    }
}

static char *get_sshd_val(const char *config, const char *key, const char *def) {
    if (!config) return g_strdup(def);
    g_autofree char *search = g_strdup_printf("\n%s ", key);
    char *match = strstr(config, search);
    if (!match) {
        // try looking at start of file
        search = g_strdup_printf("%s ", key);
        if (g_str_has_prefix(config, search)) match = (char*)config;
    }
    
    if (match) {
        match += strlen(key) + 1; // skip space
        while (*match == ' ' || *match == '\t') match++;
        char *end = strchr(match, '\n');
        if (end) {
            return g_strndup(match, end - match);
        }
        return g_strdup(match);
    }
    return g_strdup(def);
}

GtkWidget *page_ssh_new(void) {
    if (!g_find_program_in_path("sshd")) {
        return omarchy_make_unavailable_page("SSH Server", "sudo pacman -S openssh", "network-server-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "SSH Server");
    adw_preferences_page_set_icon_name(page, "network-server-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Service Status ────────────────────────────────────────────────────── */
    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "Daemon Configuration");
    adw_preferences_page_add(page, st_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Start SSH Server");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_enable), "Enable remote access via OpenSSH");
    
    g_autoptr(GError) err = NULL;
    g_autofree char *sys_out = subprocess_run_sync("systemctl is-active sshd", &err);
    gboolean is_active = (sys_out && strstr(sys_out, "active") && !strstr(sys_out, "inactive"));
    adw_switch_row_set_active(r_enable, is_active);
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_enable));

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Configuration");
    GtkWidget *save_btn = gtk_button_new_with_label("Save & Restart");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_apply_ssh), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(st_group, GTK_WIDGET(save_row));

    /* ── Settings ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *cfg_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(cfg_group, "Security & Settings");
    adw_preferences_page_add(page, cfg_group);

    // Parse effective config quickly (ignore comments for simplicity by shelling out)
    g_autofree char *cfg_out = subprocess_run_sync("grep -E '^[^#]' /etc/ssh/sshd_config", &err);

    g_autofree char *val_port = get_sshd_val(cfg_out, "Port", "22");
    g_autofree char *val_root = get_sshd_val(cfg_out, "PermitRootLogin", "prohibit-password");
    g_autofree char *val_pass = get_sshd_val(cfg_out, "PasswordAuthentication", "yes");
    g_autofree char *val_x11 = get_sshd_val(cfg_out, "X11Forwarding", "no");

    r_port = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 65535, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_port), "Listening Port");
    adw_spin_row_set_value(r_port, atoi(val_port));
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_port));

    GtkStringList *root_m = gtk_string_list_new(NULL);
    gtk_string_list_append(root_m, "Yes (Insecure)");
    gtk_string_list_append(root_m, "Keys Only (prohibit-password)");
    gtk_string_list_append(root_m, "No (Secure)");
    r_root = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_root), "Permit Root Login");
    adw_combo_row_set_model(r_root, G_LIST_MODEL(root_m));
    if (strcmp(val_root, "yes") == 0) adw_combo_row_set_selected(r_root, 0);
    else if (strcmp(val_root, "no") == 0) adw_combo_row_set_selected(r_root, 2);
    else adw_combo_row_set_selected(r_root, 1);
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_root));

    r_pass = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_pass), "Password Authentication");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_pass), "Disable this to force SSH key authentication");
    adw_switch_row_set_active(r_pass, (strcmp(val_pass, "no") != 0)); // default yes
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_pass));

    r_x11 = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_x11), "X11 Forwarding");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_x11), "Allow remote GUI applications");
    adw_switch_row_set_active(r_x11, (strcmp(val_x11, "yes") == 0));
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_x11));

    return overlay;
}
