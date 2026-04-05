#include "page-github.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwEntryRow *r_name = NULL;
static AdwEntryRow *r_email = NULL;
static AdwEntryRow *r_branch = NULL;
static AdwComboRow *r_push = NULL;
static AdwPreferencesGroup *auth_group = NULL;
static GtkWidget *auth_list_box = NULL;

static void refresh_github(void);

static void on_gh_login(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    subprocess_run_async("alacritty -t 'GitHub Login' -e gh auth login", NULL, NULL);
    g_timeout_add(5000, (GSourceFunc)refresh_github, NULL); // wait for them to login
}

static void on_gh_logout(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("gh auth logout --hostname github.com", &err);
    refresh_github();
}

static void on_save_git(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(r_name));
    const char *email = gtk_editable_get_text(GTK_EDITABLE(r_email));
    const char *branch = gtk_editable_get_text(GTK_EDITABLE(r_branch));
    
    const char *pushes[] = {"simple", "matching", "upstream", "current"};
    const char *push = pushes[adw_combo_row_get_selected(r_push)];

    g_autoptr(GError) err = NULL;
    if (strlen(name) > 0) {
        g_autofree char *cmd = g_strdup_printf("git config --global user.name '%s'", name);
        subprocess_run_sync(cmd, &err);
    }
    if (strlen(email) > 0) {
        g_autofree char *cmd = g_strdup_printf("git config --global user.email '%s'", email);
        subprocess_run_sync(cmd, &err);
    }
    if (strlen(branch) > 0) {
        g_autofree char *cmd = g_strdup_printf("git config --global init.defaultBranch '%s'", branch);
        subprocess_run_sync(cmd, &err);
    }
    
    g_autofree char *cmd_p = g_strdup_printf("git config --global push.default '%s'", push);
    subprocess_run_sync(cmd_p, &err);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Git configuration saved globally (~/.gitconfig)"));
    }
}

static void on_gen_ssh(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *email = gtk_editable_get_text(GTK_EDITABLE(r_email));
    if (strlen(email) == 0) {
        if (page_widget) {
            GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
            if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Please enter an email address first"));
        }
        return;
    }

    g_autofree char *cmd = g_strdup_printf("alacritty -t 'Generating SSH Key' -e bash -c 'ssh-keygen -t ed25519 -C \"%s\" -f ~/.ssh/id_ed25519; eval \"$(ssh-agent -s)\"; ssh-add ~/.ssh/id_ed25519; echo \"Done! Press Enter.\"; read'", email);
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_pub_ssh(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    char *out = subprocess_run_sync("gh ssh-key add ~/.ssh/id_ed25519.pub -t 'System Linux Device'", &err);
    
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            if (out && !err) {
                adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("SSH Key uploaded to GitHub successfully"));
            } else {
                adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Failed to upload key. Are you logged in to GH CLI?"));
            }
        }
    }
    g_free(out);
}

static void refresh_github(void) {
    if (!auth_list_box) return;

    gtk_list_box_remove_all(GTK_LIST_BOX(auth_list_box));

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("gh auth status", &err);
    
    gboolean is_logged_in = FALSE;
    char *username = NULL;

    if (out) {
        // "Logged in to github.com as abyandimas"
        char *ptr = strstr(out, "Logged in to github.com as ");
        if (ptr) {
            ptr += 27;
            char *end = strchr(ptr, ' ');
            if (!end) end = strchr(ptr, '\n');
            if (end) {
                username = g_strndup(ptr, end - ptr);
            } else {
                username = g_strdup(ptr);
            }
            g_strstrip(username);
            is_logged_in = TRUE;
        }
    }

    AdwActionRow *ac_row = ADW_ACTION_ROW(adw_action_row_new());
    if (is_logged_in) {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_row), g_strdup_printf("Logged in as @%s", username));
        GtkWidget *b_out = gtk_button_new_with_label("Log Out");
        gtk_widget_add_css_class(b_out, "destructive-action");
        gtk_widget_set_valign(b_out, GTK_ALIGN_CENTER);
        g_signal_connect(b_out, "clicked", G_CALLBACK(on_gh_logout), NULL);
        adw_action_row_add_suffix(ac_row, b_out);
    } else {
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_row), "Not logged into GitHub");
        GtkWidget *b_in = gtk_button_new_with_label("Log In (OAuth)");
        gtk_widget_add_css_class(b_in, "suggested-action");
        gtk_widget_set_valign(b_in, GTK_ALIGN_CENTER);
        g_signal_connect(b_in, "clicked", G_CALLBACK(on_gh_login), NULL);
        adw_action_row_add_suffix(ac_row, b_in);
    }
    gtk_list_box_append(GTK_LIST_BOX(auth_list_box), GTK_WIDGET(ac_row));
    g_free(username);
}

GtkWidget *page_github_new(void) {
    if (!g_find_program_in_path("git")) {
        return omarchy_make_unavailable_page("Git & GitHub", "sudo pacman -S git github-cli", "vcs-normal-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Git & GitHub");
    adw_preferences_page_set_icon_name(page, "vcs-normal-symbolic"); // Default icon
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── GitHub Auth ───────────────────────────────────────────────────────── */
    auth_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(auth_group, "GitHub CLI Authentication");
    
    auth_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(auth_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(auth_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(auth_group, auth_list_box);

    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *ref_b = gtk_button_new_from_icon_name("view-refresh-symbolic");
    g_signal_connect(ref_b, "clicked", G_CALLBACK(refresh_github), NULL);
    gtk_box_append(GTK_BOX(top_box), ref_b);
    adw_preferences_group_set_header_suffix(auth_group, top_box);
    adw_preferences_page_add(page, auth_group);

    refresh_github();

    /* ── Git Global Config ─────────────────────────────────────────────────── */
    AdwPreferencesGroup *cfg_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(cfg_group, "Global Git Configuration");
    adw_preferences_group_set_description(cfg_group, "Sets your identity across all repositories on this machine");
    
    GtkWidget *sb = gtk_button_new_with_label("Save Identity");
    gtk_widget_add_css_class(sb, "suggested-action");
    g_signal_connect(sb, "clicked", G_CALLBACK(on_save_git), NULL);
    adw_preferences_group_set_header_suffix(cfg_group, sb);
    adw_preferences_page_add(page, cfg_group);

    g_autofree char *v_name = subprocess_run_sync("git config --global user.name", &err);
    g_autofree char *v_email = subprocess_run_sync("git config --global user.email", &err);
    g_autofree char *v_branch = subprocess_run_sync("git config --global init.defaultBranch", &err);
    g_autofree char *v_push = subprocess_run_sync("git config --global push.default", &err);

    r_name = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_name), "Full Name (user.name)");
    if (v_name) gtk_editable_set_text(GTK_EDITABLE(r_name), g_strstrip(v_name));
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_name));

    r_email = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_email), "Email Address (user.email)");
    if (v_email) gtk_editable_set_text(GTK_EDITABLE(r_email), g_strstrip(v_email));
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_email));

    r_branch = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_branch), "Default Branch (e.g. main/master)");
    if (v_branch && strlen(v_branch)>1) gtk_editable_set_text(GTK_EDITABLE(r_branch), g_strstrip(v_branch));
    else gtk_editable_set_text(GTK_EDITABLE(r_branch), "main");
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_branch));

    GtkStringList *lm = gtk_string_list_new(NULL);
    gtk_string_list_append(lm, "Simple (Modern Default)");
    gtk_string_list_append(lm, "Matching");
    gtk_string_list_append(lm, "Upstream");
    gtk_string_list_append(lm, "Current");
    r_push = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_push), "Push Behavior");
    adw_combo_row_set_model(r_push, G_LIST_MODEL(lm));
    if (v_push) {
        if (strstr(v_push, "matching")) adw_combo_row_set_selected(r_push, 1);
        else if (strstr(v_push, "upstream")) adw_combo_row_set_selected(r_push, 2);
        else if (strstr(v_push, "current")) adw_combo_row_set_selected(r_push, 3);
        else adw_combo_row_set_selected(r_push, 0); // simple
    }
    adw_preferences_group_add(cfg_group, GTK_WIDGET(r_push));

    /* ── SSH Keys ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ssh_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(ssh_group, "SSH Key Management");
    adw_preferences_page_add(page, ssh_group);

    AdwActionRow *sr = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr), "Ed25519 Secure Shell Key");
    
    g_autofree char *ssh_path = g_build_filename(g_get_home_dir(), ".ssh", "id_ed25519.pub", NULL);
    if (g_file_test(ssh_path, G_FILE_TEST_EXISTS)) {
        adw_action_row_set_subtitle(sr, "Key exists in ~/.ssh/id_ed25519.pub");
        
        GtkWidget *b_pub = gtk_button_new_with_label("Register via GH-CLI");
        gtk_widget_add_css_class(b_pub, "suggested-action");
        gtk_widget_set_valign(b_pub, GTK_ALIGN_CENTER);
        g_signal_connect(b_pub, "clicked", G_CALLBACK(on_pub_ssh), NULL);
        adw_action_row_add_suffix(sr, b_pub);
    } else {
        adw_action_row_set_subtitle(sr, "No Ed25519 key found. You cannot push via SSH natively.");
        
        GtkWidget *b_gen = gtk_button_new_with_label("Generate Keypair");
        gtk_widget_add_css_class(b_gen, "suggested-action");
        gtk_widget_set_valign(b_gen, GTK_ALIGN_CENTER);
        g_signal_connect(b_gen, "clicked", G_CALLBACK(on_gen_ssh), NULL);
        adw_action_row_add_suffix(sr, b_gen);
    }
    adw_preferences_group_add(ssh_group, GTK_WIDGET(sr));

    return overlay;
}
