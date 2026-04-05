#include "page-pacman.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

#include <gtksourceview/gtksource.h>

static void on_pacman_save_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    GtkSourceBuffer *buffer = g_object_get_data(G_OBJECT(btn), "buffer");
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
    g_autofree char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
    
    const char *tmp_path = "/tmp/pacman.conf.tmp";
    g_file_set_contents(tmp_path, text, -1, NULL);
    g_autofree char *cmd = g_strdup_printf("pkexec bash -c \"cp %s /etc/pacman.conf && rm %s\"", tmp_path, tmp_path);
    subprocess_run_async(cmd, NULL, NULL);

    adw_dialog_close(dialog);
}

static void on_edit_pacman_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Edit pacman.conf");
    adw_dialog_set_content_width(dialog, 600);
    adw_dialog_set_content_height(dialog, 500);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkSourceView *view = GTK_SOURCE_VIEW(gtk_source_view_new());
    GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "ini");
    if (lang) gtk_source_buffer_set_language(buffer, lang);

    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_tab_width(view, 4);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);

    g_autofree char *contents = NULL;
    if (g_file_get_contents("/etc/pacman.conf", &contents, NULL, NULL) && contents) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, -1);
    } else {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "# Failed to read /etc/pacman.conf\n", -1);
    }

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(view));
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(vbox), scroll);

    GtkWidget *save_btn = gtk_button_new_with_label("Save and Apply");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_margin_top(save_btn, 8);
    gtk_widget_set_margin_bottom(save_btn, 8);
    gtk_widget_set_margin_start(save_btn, 8);
    gtk_widget_set_margin_end(save_btn, 8);

    g_object_set_data(G_OBJECT(save_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(save_btn), "buffer", buffer);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_pacman_save_clicked), NULL);

    gtk_box_append(GTK_BOX(vbox), save_btn);

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

static void on_clean_cache_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    subprocess_run_async("pkexec pacman -Sc --noconfirm", NULL, NULL);
}

static void on_update_mirrors_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    subprocess_run_async("pkexec reflector -c ID -c SG -f 10 --save /etc/pacman.d/mirrorlist", NULL, NULL);
}

GtkWidget *page_pacman_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Pacman (Arch)");
    adw_preferences_page_set_icon_name(page, "system-software-install-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Package Manager Settings");
    adw_preferences_group_set_description(group, "Configure Arch Linux package manager and mirrors.");
    adw_preferences_page_add(page, group);

    AdwActionRow *edit_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(edit_row), "Edit pacman.conf");
    adw_action_row_set_subtitle(edit_row, "Enable ParallelDownloads, Color, ILoveCandy");
    GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
    gtk_widget_set_valign(edit_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(edit_row, edit_btn);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_pacman_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(edit_row));

    AdwActionRow *cache_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cache_row), "Clean Package Cache");
    adw_action_row_set_subtitle(cache_row, "Free disk space by removing old packages");
    GtkWidget *cache_btn = gtk_button_new_with_label("Clean");
    gtk_widget_set_valign(cache_btn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(cache_btn, "destructive-action");
    adw_action_row_add_suffix(cache_row, cache_btn);
    g_signal_connect(cache_btn, "clicked", G_CALLBACK(on_clean_cache_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(cache_row));

    AdwActionRow *mirror_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mirror_row), "Update Mirrorlist");
    adw_action_row_set_subtitle(mirror_row, "Fetch fastest mirrors using Reflector");
    GtkWidget *mirror_btn = gtk_button_new_with_label("Update");
    gtk_widget_set_valign(mirror_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(mirror_row, mirror_btn);
    g_signal_connect(mirror_btn, "clicked", G_CALLBACK(on_update_mirrors_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(mirror_row));

    return GTK_WIDGET(page);
}
