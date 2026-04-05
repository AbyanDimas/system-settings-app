#include "page-starship.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static gboolean is_starship_enabled_in_bashrc(void) {
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);
    g_autofree char *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        if (strstr(contents, "starship init bash")) {
            return TRUE;
        }
    }
    return FALSE;
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
    
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);
    g_autoptr(GError) err = NULL;

    if (active) {
        if (!is_starship_enabled_in_bashrc()) {
            g_autofree char *cmd = g_strdup_printf("echo 'eval \"$(starship init bash)\"' >> \"%s\"", path);
            subprocess_run_sync(cmd, &err);
        }
    } else {
        g_autofree char *cmd = g_strdup_printf("sed -i '/starship init bash/d' \"%s\"", path);
        subprocess_run_sync(cmd, &err);
    }
}

static void on_edit_config(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *cfg = g_build_filename(g_get_home_dir(), ".config", "starship.toml", NULL);
    g_autofree char *cmd = g_strdup_printf("alacritty -t 'Editing Starship Profile' -e nvim \"%s\"", cfg);
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_install_starship(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    subprocess_run_async("alacritty -t 'Installing Starship' -e bash -c 'curl -sS https://starship.rs/install.sh | sh; echo \"Done! Press Enter.\"; read'", NULL, NULL);
}

GtkWidget *page_starship_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Terminal Prompt (Starship)");
    adw_preferences_page_set_icon_name(page, "utilities-terminal-symbolic"); // Fallback
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Shell Integration ─────────────────────────────────────────────────── */
    AdwPreferencesGroup *integ_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(integ_group, "Bash Integration");
    adw_preferences_group_set_description(integ_group, "Hook Starship into your interactive shell sessions");
    adw_preferences_page_add(page, integ_group);

    AdwSwitchRow *r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Enable Starship in ~/.bashrc");
    adw_switch_row_set_active(r_enable, is_starship_enabled_in_bashrc());
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);

    if (!g_find_program_in_path("starship")) {
        gtk_widget_set_sensitive(GTK_WIDGET(r_enable), FALSE);
        adw_action_row_set_subtitle(ADW_ACTION_ROW(r_enable), "Starship binary not found. Please install it first.");
        
        AdwActionRow *ac_in = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_in), "Install Core Framework");
        GtkWidget *b_in = gtk_button_new_with_label("Install");
        gtk_widget_add_css_class(b_in, "suggested-action");
        gtk_widget_set_valign(b_in, GTK_ALIGN_CENTER);
        g_signal_connect(b_in, "clicked", G_CALLBACK(on_install_starship), NULL);
        adw_action_row_add_suffix(ac_in, b_in);
        adw_preferences_group_add(integ_group, GTK_WIDGET(ac_in));
    } else {
        adw_preferences_group_add(integ_group, GTK_WIDGET(r_enable));
    }

    /* ── Configuration ─────────────────────────────────────────────────────── */
    AdwPreferencesGroup *cfg_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(cfg_group, "Customization");
    adw_preferences_page_add(page, cfg_group);

    AdwActionRow *ac_ed = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_ed), "Edit Prompt Configuration");
    adw_action_row_set_subtitle(ac_ed, "Modify ~/.config/starship.toml directly via Neovim");
    
    GtkWidget *b_ed = gtk_button_new_from_icon_name("document-edit-symbolic");
    gtk_widget_set_valign(b_ed, GTK_ALIGN_CENTER);
    g_signal_connect(b_ed, "clicked", G_CALLBACK(on_edit_config), NULL);
    adw_action_row_add_suffix(ac_ed, b_ed);
    
    if (!g_find_program_in_path("starship")) gtk_widget_set_sensitive(GTK_WIDGET(ac_ed), FALSE);
    adw_preferences_group_add(cfg_group, GTK_WIDGET(ac_ed));

    return overlay;
}
