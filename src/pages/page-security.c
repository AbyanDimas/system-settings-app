#include "page-security.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    AdwActionRow *row;
    GtkWidget *spinner;
    GtkWidget *icon;
    const char *title;
    const char *cmd;
} SecurityCheck;

static SecurityCheck checks[10];
static GtkWidget *page_widget = NULL;

static void free_g_free(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void set_row_status(SecurityCheck *chk, const char *status, const char *subtitle, const char *icon_name, const char *css_class) {
    g_autofree char *full_title = g_strdup_printf("%s: %s", chk->title, status);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(chk->row), full_title);
    if (subtitle) {
        adw_action_row_set_subtitle(chk->row, subtitle);
    }
    
    gtk_widget_set_visible(chk->spinner, FALSE);
    gtk_image_set_from_icon_name(GTK_IMAGE(chk->icon), icon_name);
    
    // reset classes
    gtk_widget_remove_css_class(chk->icon, "success");
    gtk_widget_remove_css_class(chk->icon, "warning");
    gtk_widget_remove_css_class(chk->icon, "error");
    if (css_class) {
        gtk_widget_add_css_class(chk->icon, css_class);
    }
    gtk_widget_set_visible(chk->icon, TRUE);
}

static void on_check_completed(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GSubprocess *proc = G_SUBPROCESS(source_object);
    SecurityCheck *chk = user_data;
    
    g_autoptr(GError) err = NULL;
    g_autofree char *stdout_buf = NULL;
    g_autofree char *stderr_buf = NULL;
    
    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, &stderr_buf, &err);
    if (stdout_buf) g_strstrip(stdout_buf);

    switch (chk->id) {
        case 0: // LUKS
            if (stdout_buf && strcmp(stdout_buf, "1") == 0) {
                set_row_status(chk, "Encrypted ✓", "LUKS volumes detected", "emblem-ok-symbolic", "success");
            } else {
                set_row_status(chk, "No encrypted volumes ⚠", "Disk may be unencrypted", "dialog-warning-symbolic", "warning");
            }
            break;
        case 1: // Firewall
            if (stdout_buf && strcmp(stdout_buf, "inactive") == 0) {
                set_row_status(chk, "Inactive ✗", "Firewall is not running", "dialog-error-symbolic", "error");
            } else if (stdout_buf) {
                g_autofree char *sub = g_strdup_printf("Active (%s rules)", stdout_buf);
                set_row_status(chk, "Active ✓", sub, "emblem-ok-symbolic", "success");
            }
            break;
        case 2: // SSH
            if (stdout_buf && strcmp(stdout_buf, "no") == 0) {
                set_row_status(chk, "Running (Secure) ✓", "SSHD active", "emblem-ok-symbolic", "success");
            } else if (stdout_buf && strstr(stdout_buf, "inactive")) {
                set_row_status(chk, "Not Running", "SSHD disabled", "emblem-ok-symbolic", NULL);
            } else {
                set_row_status(chk, "Running (Insecure!) ✗", "Root/Password auth enabled", "dialog-error-symbolic", "error");
            }
            break;
        case 3: // Sudo
            if (stdout_buf && strcmp(stdout_buf, "ok") == 0) {
                set_row_status(chk, "Password required ✓", "No NOPASSWD entries found", "emblem-ok-symbolic", "success");
            } else {
                set_row_status(chk, "NOPASSWD found ⚠", "Sudo allows passwordless execution", "dialog-warning-symbolic", "warning");
            }
            break;
        case 4: // Failed Logins
            if (stdout_buf) {
                int count = atoi(stdout_buf);
                if (count > 10) {
                    g_autofree char *sub = g_strdup_printf("%d failed attempts this week", count);
                    set_row_status(chk, "High Auth Failures ✗", sub, "dialog-error-symbolic", "error");
                } else if (count > 0) {
                    g_autofree char *sub = g_strdup_printf("%d failed attempts this week", count);
                    set_row_status(chk, "Auth Failures ⚠", sub, "dialog-warning-symbolic", "warning");
                } else {
                    set_row_status(chk, "Clean ✓", "0 failed SSH attempts", "emblem-ok-symbolic", "success");
                }
            }
            break;
        case 5: // Updates
            if (stdout_buf) {
                int count = atoi(stdout_buf);
                if (count == 0) {
                    set_row_status(chk, "Up to date ✓", "No pending pacman updates", "emblem-ok-symbolic", "success");
                } else {
                    g_autofree char *sub = g_strdup_printf("%d updates available", count);
                    set_row_status(chk, "Updates Available ⚠", sub, "dialog-warning-symbolic", "warning");
                }
            }
            break;
        case 6: // Fingerprint
            if (stdout_buf && strcmp(stdout_buf, "1") == 0) {
                set_row_status(chk, "Enrolled ✓", "Biometrics configured", "emblem-ok-symbolic", "success");
            } else {
                set_row_status(chk, "Not configured", "No fingers enrolled", "dialog-information-symbolic", NULL);
            }
            break;
        case 7: // Open Ports
            if (stdout_buf) {
                int count = atoi(stdout_buf) - 1; // subtract header
                if (count < 0) count = 0;
                g_autofree char *sub = g_strdup_printf("%d listening TCP ports", count);
                set_row_status(chk, "Active Listening Ports", sub, "network-server-symbolic", "success");
            }
            break;
        case 8: // GPG
            if (stdout_buf) {
                g_autofree char *sub = g_strdup_printf("%s GPG keys in keychain", stdout_buf);
                set_row_status(chk, "Keys Present ✓", sub, "emblem-ok-symbolic", "success");
            }
            break;
        case 9: // Pacman Keyring
            if (stdout_buf && strcmp(stdout_buf, "0") == 0) {
                set_row_status(chk, "All keys valid ✓", "Pacman keyring healthy", "emblem-ok-symbolic", "success");
            } else {
                set_row_status(chk, "Expired keys found ⚠", "Run pacman-key --refresh-keys", "dialog-warning-symbolic", "warning");
            }
            break;
    }
}

static void run_all_checks(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;

    for (int i = 0; i < 10; i++) {
        SecurityCheck *chk = &checks[i];
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(chk->row), g_strdup_printf("%s: Checking...", chk->title));
        adw_action_row_set_subtitle(chk->row, "");
        gtk_widget_set_visible(chk->spinner, TRUE);
        gtk_widget_set_visible(chk->icon, FALSE);

        g_autoptr(GError) err = NULL;
        const char *argv[] = {"sh", "-c", chk->cmd, NULL};
        GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &err);
        if (proc) {
            g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_check_completed, chk);
        } else {
            set_row_status(chk, "Execution Failed", "Could not run check", "dialog-error-symbolic", "error");
        }
    }
}

GtkWidget *page_security_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Security Overview");
    adw_preferences_page_set_icon_name(page, "security-high-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *hdr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, hdr_group);

    AdwActionRow *run_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(run_row), "Auditing System");
    GtkWidget *run_btn = gtk_button_new_with_label("Re-run Checks");
    gtk_widget_set_valign(run_btn, GTK_ALIGN_CENTER);
    g_signal_connect(run_btn, "clicked", G_CALLBACK(run_all_checks), NULL);
    adw_action_row_add_suffix(run_row, run_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(run_row));

    AdwPreferencesGroup *dash_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(dash_group, "Dashboard");
    adw_preferences_page_add(page, dash_group);

    const char *titles[] = {
        "Disk Encryption", "Firewall", "SSH Daemon", "Sudo Access",
        "Failed Logins", "System Updates", "Fingerprint Auth",
        "Open Ports", "GPG Keys", "Pacman Keyring"
    };
    const char *cmds[] = {
        "lsblk -o TYPE | grep -q crypt && echo 1 || echo 0",
        "if command -v ufw >/dev/null; then ufw status | grep -q active && ufw status | wc -l || echo inactive; else echo inactive; fi",
        "systemctl is-active --quiet sshd || (echo inactive && exit 0); grep -Eq '^(PermitRootLogin|PasswordAuthentication) yes' /etc/ssh/sshd_config && echo insecure || echo no",
        "sudo -n -l -U $USER 2>/dev/null | grep -q NOPASSWD && echo warning || echo ok",
        "journalctl -u sshd --since '7 days ago' 2>/dev/null | grep -c 'Failed password' || echo 0",
        "checkupdates 2>/dev/null | wc -l || echo 0",
        "if command -v fprintd-list >/dev/null; then fprintd-list $USER 2>/dev/null | grep -q 'Yes' && echo 1 || echo 0; else echo 0; fi",
        "ss -tlnp | wc -l",
        "gpg --list-keys 2>/dev/null | grep -c '^pub '",
        "pacman-key --list-keys 2>/dev/null | grep -c expired || echo 0"
    };

    for (int i = 0; i < 10; i++) {
        checks[i].id = i;
        checks[i].title = titles[i];
        checks[i].cmd = cmds[i];
        
        checks[i].row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(checks[i].row), titles[i]);
        
        checks[i].spinner = gtk_spinner_new();
        gtk_spinner_start(GTK_SPINNER(checks[i].spinner));
        gtk_widget_set_valign(checks[i].spinner, GTK_ALIGN_CENTER);
        adw_action_row_add_prefix(checks[i].row, checks[i].spinner);

        checks[i].icon = gtk_image_new_from_icon_name("emblem-synchronizing-symbolic");
        gtk_widget_set_valign(checks[i].icon, GTK_ALIGN_CENTER);
        gtk_widget_set_size_request(checks[i].icon, 32, 32); // fixed size ensures text doesn't jump
        gtk_widget_set_visible(checks[i].icon, FALSE);
        adw_action_row_add_prefix(checks[i].row, checks[i].icon);

        adw_preferences_group_add(dash_group, GTK_WIDGET(checks[i].row));
    }

    // Auto-start checks layout complete
    run_all_checks(NULL, NULL);

    return overlay;
}
