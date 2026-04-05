#include "page-gamemode.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwSwitchRow *r_enable = NULL;

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    g_autoptr(GError) err = NULL;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        subprocess_run_async("systemctl --user enable --now gamemoded", NULL, NULL);
    } else {
        subprocess_run_async("systemctl --user disable --now gamemoded", NULL, NULL);
    }
}

static void on_test_gamemode(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *cmd = g_strdup("alacritty -t 'GameMode Diagnostics' -e bash -c 'gamemoded -t; read -p \"Press Enter to exit\"'");
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_gamemode_new(void) {
    if (!g_find_program_in_path("gamemoded")) {
        return omarchy_make_unavailable_page("Gaming & GameMode", "sudo pacman -S gamemode lib32-gamemode", "input-gaming-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Gaming & GameMode");
    adw_preferences_page_set_icon_name(page, "input-gaming-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── GameMode (Feral Interactive) ──────────────────────────────────────── */
    AdwPreferencesGroup *gm_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(gm_group, "Feral GameMode");
    adw_preferences_group_set_description(gm_group, "Optimizes Linux system performance on demand for gaming.");
    adw_preferences_page_add(page, gm_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Enable GameMode Daemon");
    
    g_autofree char *st_out = subprocess_run_sync("systemctl --user is-active gamemoded", &err);
    adw_switch_row_set_active(r_enable, (st_out && strstr(st_out, "active") && !strstr(st_out, "inactive")));
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    
    adw_preferences_group_add(gm_group, GTK_WIDGET(r_enable));

    AdwActionRow *ac_test = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_test), "Run Diagnostics");
    adw_action_row_set_subtitle(ac_test, "Verify the daemon is running and has permissions to set CPU governors");
    GtkWidget *b_test = gtk_button_new_with_label("Test GameMode");
    gtk_widget_add_css_class(b_test, "suggested-action");
    gtk_widget_set_valign(b_test, GTK_ALIGN_CENTER);
    g_signal_connect(b_test, "clicked", G_CALLBACK(on_test_gamemode), NULL);
    adw_action_row_add_suffix(ac_test, b_test);
    adw_preferences_group_add(gm_group, GTK_WIDGET(ac_test));

    /* ── Gamescope ─────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *gs_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(gs_group, "Gamescope Micro-Compositor");
    adw_preferences_page_add(page, gs_group);

    AdwActionRow *gs_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gs_row), "Gamescope Status");
    if (g_find_program_in_path("gamescope")) {
        adw_action_row_set_subtitle(gs_row, "Installed. Run games with Steam Launch Options: gamescope -W 2560 -H 1440 -f -- %command%");
    } else {
        adw_action_row_set_subtitle(gs_row, "Not strictly necessary. Install it with sudo pacman -S gamescope if required for upscaling.");
    }
    adw_preferences_group_add(gs_group, GTK_WIDGET(gs_row));

    /* ── Custom Protons ────────────────────────────────────────────────────── */
    AdwPreferencesGroup *pr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(pr_group, "Custom Proton Builds (Steam)");
    adw_preferences_page_add(page, pr_group);

    g_autofree char *target_dir = g_build_filename(g_get_home_dir(), ".local", "share", "Steam", "compatibilitytools.d", NULL);
    
    int proton_count = 0;
    if (g_file_test(target_dir, G_FILE_TEST_IS_DIR)) {
        g_autofree char *cmd_ls = g_strdup_printf("ls -1 \"%s\"", target_dir);
        g_autofree char *ls_out = subprocess_run_sync(cmd_ls, &err);
        
        if (ls_out) {
            g_auto(GStrv) lines = g_strsplit(ls_out, "\n", -1);
            for (int i = 0; lines[i] != NULL; i++) {
                char *line = g_strstrip(g_strdup(lines[i]));
                if (strlen(line) > 0) {
                    AdwActionRow *pr_row = ADW_ACTION_ROW(adw_action_row_new());
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pr_row), line);
                    adw_preferences_group_add(pr_group, GTK_WIDGET(pr_row));
                    proton_count++;
                }
                g_free(line);
            }
        }
    }

    if (proton_count == 0) {
        AdwActionRow *pr_row2 = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pr_row2), "No GE-Proton tools installed.");
        adw_action_row_set_subtitle(pr_row2, "You can install community builds via ProtonUp-Qt.");
        adw_preferences_group_add(pr_group, GTK_WIDGET(pr_row2));
    }

    /* ── Wine ──────────────────────────────────────────────────────────────── */
    AdwActionRow *w_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(w_row), "Wine Diagnostics");
    if (g_find_program_in_path("wine")) {
        g_autofree char *w_ver = subprocess_run_sync("wine --version", &err);
        adw_action_row_set_subtitle(w_row, w_ver ? g_strstrip(w_ver) : "Installed");
    } else {
        adw_action_row_set_subtitle(w_row, "Wine is not installed globally.");
    }
    adw_preferences_group_add(pr_group, GTK_WIDGET(w_row));

    return overlay;
}
