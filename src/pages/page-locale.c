#include "page-locale.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

static void on_sync_time_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("pkexec timedatectl set-ntp true", &err);
}

#include <gtksourceview/gtksource.h>

static void on_locale_save_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    GtkSourceBuffer *buffer = g_object_get_data(G_OBJECT(btn), "buffer");
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
    g_autofree char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
    
    g_autofree char *tmp_path = g_strdup_printf("/tmp/omarchy_locale_%d.tmp", g_random_int());
    g_file_set_contents(tmp_path, text, -1, NULL);
    
    g_autofree char *cmd = g_strdup_printf("pkexec bash -c \"cp %s /etc/locale.gen && rm %s && locale-gen\"", tmp_path, tmp_path);
    subprocess_run_async(cmd, NULL, NULL);

    adw_dialog_close(dialog);
}

static void on_edit_locale_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Edit Locale Configuration");
    adw_dialog_set_content_width(dialog, 600);
    adw_dialog_set_content_height(dialog, 500);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkSourceView *view = GTK_SOURCE_VIEW(gtk_source_view_new());
    GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "sh");
    if (lang) gtk_source_buffer_set_language(buffer, lang);

    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_tab_width(view, 4);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);

    g_autofree char *contents = NULL;
    if (g_file_get_contents("/etc/locale.gen", &contents, NULL, NULL) && contents) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, -1);
    } else {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "# Failed to read /etc/locale.gen\n", -1);
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
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_locale_save_clicked), NULL);

    gtk_box_append(GTK_BOX(vbox), save_btn);

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

static void on_timezone_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    GListModel *model = adw_combo_row_get_model(row);
    guint selected = adw_combo_row_get_selected(row);
    const char *tz = gtk_string_list_get_string(GTK_STRING_LIST(model), selected);
    if (!tz) return;

    g_autofree char *cmd = g_strdup_printf("pkexec timedatectl set-timezone '%s'", tz);
    subprocess_run_async(cmd, NULL, NULL);
}

static AdwComboRow* create_timezone_combo(void) {
    AdwComboRow *row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "System Timezone");
    adw_combo_row_set_enable_search(row, TRUE);

    GtkStringList *model = gtk_string_list_new(NULL);
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("timedatectl list-timezones", &err);
    
    int default_idx = 0;
    int count = 0;
    g_autoptr(GError) err2 = NULL;
    g_autofree char *tz_link = g_file_read_link("/etc/localtime", &err2);
    char *curr_tz = NULL;
    if (tz_link) {
        char *p = strstr(tz_link, "zoneinfo/");
        if (p) curr_tz = p + 9;
    }

    if (out) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i=0; lines[i] != NULL; i++) {
            if (strlen(lines[i]) < 3) continue;
            gtk_string_list_append(model, lines[i]);
            if (curr_tz && strcmp(curr_tz, lines[i]) == 0) {
                default_idx = count;
            }
            count++;
        }
    }

    adw_combo_row_set_model(row, G_LIST_MODEL(model));
    if (count > 0) adw_combo_row_set_selected(row, default_idx);

    g_signal_connect(row, "notify::selected", G_CALLBACK(on_timezone_selected), NULL);
    return row;
}

GtkWidget *page_locale_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Time & Language");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-locale-symbolic");

    AdwPreferencesGroup *time_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(time_group, "Date & Time");
    adw_preferences_group_set_description(time_group, "Configure system clock and timezone settings.");
    adw_preferences_page_add(page, time_group);

    AdwActionRow *ntp_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ntp_row), "Automatic Time (NTP)");
    adw_action_row_set_subtitle(ntp_row, "Sync system clock via network");
    GtkWidget *ntp_btn = gtk_button_new_with_label("Enable NTP");
    gtk_widget_set_valign(ntp_btn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(ntp_btn, "suggested-action");
    adw_action_row_add_suffix(ntp_row, ntp_btn);
    g_signal_connect(ntp_btn, "clicked", G_CALLBACK(on_sync_time_clicked), NULL);
    adw_preferences_group_add(time_group, GTK_WIDGET(ntp_row));

    AdwComboRow *tz_row = create_timezone_combo();
    adw_action_row_set_subtitle(ADW_ACTION_ROW(tz_row), "Search and apply timezone (requires authentication)");
    adw_preferences_group_add(time_group, GTK_WIDGET(tz_row));

    AdwPreferencesGroup *loc_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(loc_group, "System Locale");
    adw_preferences_page_add(page, loc_group);

    AdwActionRow *edit_loc_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(edit_loc_row), "Edit Locale Configuration");
    adw_action_row_set_subtitle(edit_loc_row, "Modify /etc/locale.gen and generate locales");
    GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
    gtk_widget_set_valign(edit_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(edit_loc_row, edit_btn);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_locale_clicked), NULL);
    adw_preferences_group_add(loc_group, GTK_WIDGET(edit_loc_row));

    return GTK_WIDGET(page);
}
