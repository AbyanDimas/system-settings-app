#include "page-defaultapps.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

#include <gtksourceview/gtksource.h>

static void on_browser_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    GListModel *model = adw_combo_row_get_model(row);
    guint selected = adw_combo_row_get_selected(row);
    const char *browser_desktop = gtk_string_list_get_string(GTK_STRING_LIST(model), selected);
    if (!browser_desktop) return;

    g_autofree char *cmd = g_strdup_printf("xdg-settings set default-web-browser '%s'", browser_desktop);
    subprocess_run_async(cmd, NULL, NULL);
}

static AdwComboRow* create_browser_combo(void) {
    AdwComboRow *row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Web Browser");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Default app for opening websites (xdg-settings)");

    GtkStringList *model = gtk_string_list_new(NULL);
    
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("sh -c 'grep -Rl \"x-scheme-handler/http\" /usr/share/applications/ ~/.local/share/applications/ 2>/dev/null | awk -F/ \"{print \\$NF}\" | sort -u'", &err);
    
    int default_idx = 0;
    int count = 0;
    g_autoptr(GError) err2 = NULL;
    g_autofree char *def_browser = subprocess_run_sync("xdg-settings get default-web-browser", &err2);
    if (def_browser) g_strstrip(def_browser);

    if (out) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i=0; lines[i] != NULL; i++) {
            if (strlen(lines[i]) < 4) continue;
            gtk_string_list_append(model, lines[i]);
            if (def_browser && strcmp(def_browser, lines[i]) == 0) {
                default_idx = count;
            }
            count++;
        }
    }
    adw_combo_row_set_model(row, G_LIST_MODEL(model));
    if (count > 0) adw_combo_row_set_selected(row, default_idx);
    
    g_signal_connect(row, "notify::selected", G_CALLBACK(on_browser_selected), NULL);
    return row;
}

static void on_mime_save_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    GtkSourceBuffer *buffer = g_object_get_data(G_OBJECT(btn), "buffer");
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
    g_autofree char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
    
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "mimeapps.list", NULL);
    g_file_set_contents(path, text, -1, NULL);

    adw_dialog_close(dialog);
}

static void on_edit_mime_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "MIME Type Associations");
    adw_dialog_set_content_width(dialog, 600);
    adw_dialog_set_content_height(dialog, 500);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkSourceView *view = GTK_SOURCE_VIEW(gtk_source_view_new());
    GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "ini");
    if (lang) gtk_source_buffer_set_language(buffer, lang);

    GtkSourceStyleSchemeManager *scheme_mgr = gtk_source_style_scheme_manager_get_default();
    AdwStyleManager *adw_mgr = adw_style_manager_get_default();
    const char *scheme_id = adw_style_manager_get_dark(adw_mgr) ? "Adwaita-dark" : "Adwaita";
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_mgr, scheme_id);
    if (scheme) gtk_source_buffer_set_style_scheme(buffer, scheme);

    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_tab_width(view, 4);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "mimeapps.list", NULL);
    g_autofree char *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL) && contents) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, -1);
    } else {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "[Default Applications]\n", -1);
    }

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(view));
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(vbox), scroll);

    GtkWidget *save_btn = gtk_button_new_with_label("Save and Close");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_margin_top(save_btn, 8);
    gtk_widget_set_margin_bottom(save_btn, 8);
    gtk_widget_set_margin_start(save_btn, 8);
    gtk_widget_set_margin_end(save_btn, 8);

    g_object_set_data(G_OBJECT(save_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(save_btn), "buffer", buffer);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_mime_save_clicked), NULL);

    gtk_box_append(GTK_BOX(vbox), save_btn);

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

GtkWidget *page_defaultapps_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Default Apps");
    adw_preferences_page_set_icon_name(page, "applications-system-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Default Applications");
    adw_preferences_group_set_description(group, "Configure system default applications and file associations.");
    adw_preferences_page_add(page, group);

    AdwComboRow *browser_row = create_browser_combo();
    if (browser_row) adw_preferences_group_add(group, GTK_WIDGET(browser_row));

    AdwActionRow *mime_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mime_row), "MIME Type Associations");
    adw_action_row_set_subtitle(mime_row, "Directly edit ~/.config/mimeapps.list");
    GtkWidget *mime_btn = gtk_button_new_with_label("Edit File");
    gtk_widget_set_valign(mime_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(mime_row, mime_btn);
    g_signal_connect(mime_btn, "clicked", G_CALLBACK(on_edit_mime_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(mime_row));

    return GTK_WIDGET(page);
}
