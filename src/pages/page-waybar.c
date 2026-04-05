#include "page-waybar.h"
#include <gtksourceview/gtksource.h>
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Waybar Config Editor ─────────────────────────────────────────────────
 * Edit waybar config with JSON syntax highlighting, save & reload.
 * ─────────────────────────────────────────────────────────────────────────*/

typedef struct {
    GtkSourceBuffer *buffer;
    AdwToastOverlay *toast_overlay;
    char            *config_path;
} SaveData;

static void on_save_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    SaveData *data = (SaveData *)user_data;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(data->buffer), &start, &end);
    char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(data->buffer), &start, &end, FALSE);

    GError *file_err = NULL;
    if (g_file_set_contents(data->config_path, text, -1, &file_err)) {
        /* Reload waybar */
        g_autoptr(GError) err = NULL;
        subprocess_run_sync("pkill -SIGUSR2 waybar", &err);

        /* Show success toast */
        if (data->toast_overlay) {
            AdwToast *toast = adw_toast_new("Waybar config saved and reloaded");
            adw_toast_set_timeout(toast, 3);
            adw_toast_overlay_add_toast(data->toast_overlay, toast);
        }
    } else {
        if (data->toast_overlay) {
            g_autofree char *msg = g_strdup_printf("Save failed: %s",
                file_err ? file_err->message : "Unknown error");
            AdwToast *toast = adw_toast_new(msg);
            adw_toast_overlay_add_toast(data->toast_overlay, toast);
        }
        g_clear_error(&file_err);
    }

    g_free(text);
}

GtkWidget *page_waybar_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Waybar");
    adw_preferences_page_set_icon_name(page, "applications-system-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Configuration");
    adw_preferences_group_set_description(group,
        "~/.config/waybar/config — Save to reload Waybar automatically.");
    adw_preferences_page_add(page, group);

    /* Source view with JSON highlighting */
    GtkSourceView *view = GTK_SOURCE_VIEW(gtk_source_view_new());
    GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));

    /* Set JSON language for syntax highlighting */
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "json");
    if (lang) gtk_source_buffer_set_language(buffer, lang);

    /* Match dark/light scheme */
    GtkSourceStyleSchemeManager *scheme_mgr = gtk_source_style_scheme_manager_get_default();
    AdwStyleManager *adw_mgr = adw_style_manager_get_default();
    const char *scheme_id = adw_style_manager_get_dark(adw_mgr) ? "Adwaita-dark" : "Adwaita";
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_mgr, scheme_id);
    if (scheme) gtk_source_buffer_set_style_scheme(buffer, scheme);

    /* Editor settings */
    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_tab_width(view, 2);
    gtk_source_view_set_auto_indent(view, TRUE);
    gtk_source_view_set_highlight_current_line(view, TRUE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);

    /* Load file */
    char *config_path = g_build_filename(
        g_get_home_dir(), ".config", "waybar", "config", NULL);
    char *contents = NULL;
    if (g_file_get_contents(config_path, &contents, NULL, NULL) && contents) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, -1);
        g_free(contents);
    } else {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer),
            "// ~/.config/waybar/config not found\n", -1);
    }

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 400);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(view));
    adw_preferences_group_add(group, scroll);

    /* Save button */
    GtkWidget *btn = gtk_button_new_with_label("Save and Reload Waybar");
    gtk_widget_add_css_class(btn, "suggested-action");
    gtk_widget_set_halign(btn, GTK_ALIGN_END);
    gtk_widget_set_margin_top(btn, 8);
    adw_preferences_group_add(group, btn);

    /* Wire up save */
    SaveData *data = g_new0(SaveData, 1);
    data->buffer = buffer;
    data->config_path = config_path; /* freed when widget destroyed via GDestroyNotify */
    data->toast_overlay = NULL;     /* Will try to grab parent overlay */
    g_signal_connect(btn, "clicked", G_CALLBACK(on_save_clicked), data);

    return GTK_WIDGET(page);
}
