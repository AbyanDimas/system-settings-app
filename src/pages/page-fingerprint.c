#include "page-fingerprint.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *fp_group = NULL;
static AdwPreferencesGroup *fido_group = NULL;

static void refresh_biometrics(void);

static void on_enroll_fp(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    // Launch terminal for interactive swiping
    subprocess_run_async("alacritty -t 'Fingerprint Enrollment' -e bash -c 'echo \"Swipe your finger to enroll...\"; fprintd-enroll; echo \"Done. Press Enter to close.\"; read'", NULL, NULL);
}

static void on_verify_fp(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_async("alacritty -t 'Fingerprint Verification' -e bash -c 'echo \"Swipe your finger to verify...\"; fprintd-verify; echo \"Done. Press Enter to close.\"; read'", NULL, NULL);
}

static void on_add_fido(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    // pamu2fcfg requires touching the physical token
    subprocess_run_async("alacritty -t 'FIDO2 Setup' -e bash -c 'echo \"Touch your security key now...\"; mkdir -p ~/.config/Yubico; pamu2fcfg >> ~/.config/Yubico/u2f_keys; echo \"Key registered! Press Enter.\"; read'", NULL, NULL);
}

static void on_clear_fido(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("rm -f ~/.config/Yubico/u2f_keys", &err);
    refresh_biometrics();
}

static void refresh_biometrics(void) {
    // Clear dynamic children except static buttons
}

GtkWidget *page_fingerprint_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    gboolean has_fprintd = caps->has_fprintd;
    gboolean has_pam_u2f = g_find_program_in_path("pamu2fcfg") != NULL;

    if (!has_fprintd && !has_pam_u2f) {
        return omarchy_make_unavailable_page("Biometrics & Security Keys", "sudo pacman -S fprintd pam-u2f", "security-high-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Fingerprint & FIDO2");
    adw_preferences_page_set_icon_name(page, "security-high-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Fingerprint ───────────────────────────────────────────────────────── */
    if (has_fprintd) {
        fp_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(fp_group, "Fingerprint Authentication");
        adw_preferences_page_add(page, fp_group);

        AdwActionRow *enroll_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(enroll_row), "Enroll New Fingerprint");
        adw_action_row_set_subtitle(enroll_row, "Opens terminal to swipe finger");
        GtkWidget *enroll_btn = gtk_button_new_with_label("Enroll");
        gtk_widget_set_valign(enroll_btn, GTK_ALIGN_CENTER);
        g_signal_connect(enroll_btn, "clicked", G_CALLBACK(on_enroll_fp), NULL);
        adw_action_row_add_suffix(enroll_row, enroll_btn);
        adw_preferences_group_add(fp_group, GTK_WIDGET(enroll_row));

        AdwActionRow *verify_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(verify_row), "Verify Fingerprint");
        GtkWidget *verify_btn = gtk_button_new_with_label("Test");
        gtk_widget_set_valign(verify_btn, GTK_ALIGN_CENTER);
        g_signal_connect(verify_btn, "clicked", G_CALLBACK(on_verify_fp), NULL);
        adw_action_row_add_suffix(verify_row, verify_btn);
        adw_preferences_group_add(fp_group, GTK_WIDGET(verify_row));

        g_autoptr(GError) err = NULL;
        const char *user = g_get_user_name();
        g_autofree char *cmd = g_strdup_printf("fprintd-list %s", user);
        g_autofree char *out = subprocess_run_sync(cmd, &err);
        
        if (out && strstr(out, "Yes")) {
            // It says "Yes" somewhere typically if enrolled or lists fingers
            AdwActionRow *stat_row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(stat_row), "Status: Fingers Enrolled");
            adw_preferences_group_add(fp_group, GTK_WIDGET(stat_row));
        } else {
            AdwActionRow *stat_row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(stat_row), "Status: No fingers enrolled");
            adw_preferences_group_add(fp_group, GTK_WIDGET(stat_row));
        }
    }

    /* ── FIDO2 ─────────────────────────────────────────────────────────────── */
    if (has_pam_u2f) {
        fido_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(fido_group, "FIDO2 Security Keys (YubiKey etc)");
        adw_preferences_page_add(page, fido_group);

        AdwActionRow *add_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_row), "Register Security Key");
        adw_action_row_set_subtitle(add_row, "Requires touching the physical key");
        GtkWidget *add_btn = gtk_button_new_with_label("Register");
        gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_fido), NULL);
        adw_action_row_add_suffix(add_row, add_btn);
        adw_preferences_group_add(fido_group, GTK_WIDGET(add_row));

        g_autofree char *keys_file = g_build_filename(g_get_home_dir(), ".config", "Yubico", "u2f_keys", NULL);
        if (g_file_test(keys_file, G_FILE_TEST_EXISTS)) {
            AdwActionRow *reg_row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(reg_row), "Keys Configured");
            
            GtkWidget *clr_btn = gtk_button_new_with_label("Clear All");
            gtk_widget_add_css_class(clr_btn, "destructive-action");
            gtk_widget_set_valign(clr_btn, GTK_ALIGN_CENTER);
            g_signal_connect(clr_btn, "clicked", G_CALLBACK(on_clear_fido), NULL);
            adw_action_row_add_suffix(reg_row, clr_btn);
            adw_preferences_group_add(fido_group, GTK_WIDGET(reg_row));
        } else {
            AdwActionRow *reg_row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(reg_row), "No keys configured");
            adw_preferences_group_add(fido_group, GTK_WIDGET(reg_row));
        }
    }

    /* ── PAM ───────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *pam_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(pam_group, "PAM Configuration (Sudo/Login)");
    adw_preferences_page_add(page, pam_group);

    AdwActionRow *pam_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pam_row), "Apply Biometrics to Sudo");
    adw_action_row_set_subtitle(pam_row, "Requires manual /etc/pam.d/sudo configuration or authselect");
    GtkWidget *term_btn = gtk_button_new_with_label("Configure PAM");
    gtk_widget_set_valign(term_btn, GTK_ALIGN_CENTER);
    g_signal_connect_swapped(term_btn, "clicked", G_CALLBACK(subprocess_run_async), "alacritty -e sudo nano /etc/pam.d/sudo");
    adw_action_row_add_suffix(pam_row, term_btn);
    adw_preferences_group_add(pam_group, GTK_WIDGET(pam_row));

    return overlay;
}
