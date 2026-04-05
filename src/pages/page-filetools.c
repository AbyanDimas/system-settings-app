#include "page-filetools.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void on_launch_search(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *cmd = user_data;
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_filetools_new(void) {
    if (!g_find_program_in_path("fd") || !g_find_program_in_path("fzf")) {
        return omarchy_make_unavailable_page("File Utilities", "sudo pacman -S fd fzf", "system-search-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "File Tools");
    adw_preferences_page_set_icon_name(page, "system-search-symbolic"); // Fallback
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── Fast Search ───────────────────────────────────────────────────────── */
    AdwPreferencesGroup *srch_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(srch_group, "Fast Fuzzy Finding (fd + fzf)");
    adw_preferences_page_add(page, srch_group);

    AdwActionRow *ac_all = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_all), "Search All Local Files");
    adw_action_row_set_subtitle(ac_all, "Interactively pipe fd directories through fzf");
    
    GtkWidget *b_all = gtk_button_new_from_icon_name("system-search-symbolic");
    gtk_widget_add_css_class(b_all, "suggested-action");
    gtk_widget_set_valign(b_all, GTK_ALIGN_CENTER);
    g_signal_connect_data(b_all, "clicked", G_CALLBACK(on_launch_search), g_strdup("alacritty -t 'Fuzzy File Search' -e bash -c 'cd ~ && export FZF_DEFAULT_COMMAND=\"fd --type f\" && fzf; read -p \"Done. Press Enter...\"'"), (GClosureNotify)free_g_free, 0);
    adw_action_row_add_suffix(ac_all, b_all);
    adw_preferences_group_add(srch_group, GTK_WIDGET(ac_all));

    AdwActionRow *ac_big = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_big), "Find Large Files (>1GB)");
    adw_action_row_set_subtitle(ac_big, "Filter files strictly larger than 1 Gigabyte");
    
    GtkWidget *b_big = gtk_button_new_from_icon_name("system-search-symbolic");
    gtk_widget_add_css_class(b_big, "destructive-action");
    gtk_widget_set_valign(b_big, GTK_ALIGN_CENTER);
    g_signal_connect_data(b_big, "clicked", G_CALLBACK(on_launch_search), g_strdup("alacritty -t 'Large Files' -e bash -c 'cd ~ && fd -S +1G | fzf; read -p \"Done. Press Enter...\"'"), (GClosureNotify)free_g_free, 0);
    adw_action_row_add_suffix(ac_big, b_big);
    adw_preferences_group_add(srch_group, GTK_WIDGET(ac_big));

    /* ── Archives ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *arc_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(arc_group, "Compression Capabilities");
    adw_preferences_group_set_description(arc_group, "Supported archive formats natively available in CLI tools");
    adw_preferences_page_add(page, arc_group);

    const char *tools[] = {"unzip", "tar", "7z", "zstd"};
    const char *names[] = {"ZIP Extractor (.zip)", "Tape Archiver (.tar.gz, .tar.xz)", "7-Zip Archives (.7z)", "Zstandard (.zst)"};

    for (int i=0; i<4; i++) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), names[i]);
        if (g_find_program_in_path(tools[i])) {
            adw_action_row_set_subtitle(row, g_strdup_printf("Installed (%s)", tools[i]));
            GtkWidget *ic = gtk_image_new_from_icon_name("emblem-ok-symbolic");
            gtk_widget_add_css_class(ic, "success");
            adw_action_row_add_suffix(row, ic);
        } else {
            adw_action_row_set_subtitle(row, g_strdup_printf("Missing (%s)", tools[i]));
            GtkWidget *ic = gtk_image_new_from_icon_name("dialog-warning-symbolic");
            gtk_widget_add_css_class(ic, "warning");
            adw_action_row_add_suffix(row, ic);
        }
        adw_preferences_group_add(arc_group, GTK_WIDGET(row));
    }

    return overlay;
}
