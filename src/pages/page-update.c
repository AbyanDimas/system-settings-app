#include "page-update.h"
#include "../window.h"
#include "../utils/subprocess.h"
#include <adwaita.h>
#include <gio/gio.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static GtkTextBuffer *out_buffer = NULL;
static GtkWidget *btn_check = NULL;
static GtkWidget *btn_update = NULL;
static GSubprocess *up_proc = NULL;
static GCancellable *up_cancel = NULL;

static void append_output(const char *text) {
    if (!out_buffer) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(out_buffer, &end);
    gtk_text_buffer_insert(out_buffer, &end, text, -1);
}

static void on_read_line(GObject *source, GAsyncResult *res, gpointer user_data) {
    GDataInputStream *stream = G_DATA_INPUT_STREAM(source);
    g_autoptr(GError) err = NULL;
    gsize length;
    char *line = g_data_input_stream_read_line_finish(stream, res, &length, &err);
    if (line) {
        g_autofree char *full = g_strdup_printf("%s\n", line);
        append_output(full);
        g_free(line);
        // Continue reading
        g_data_input_stream_read_line_async(stream, G_PRIORITY_DEFAULT, up_cancel, on_read_line, NULL);
    } else {
        // EOF or error
        gtk_widget_set_sensitive(btn_check, TRUE);
        gtk_widget_set_sensitive(btn_update, TRUE);
        append_output("\n--- Process Finished ---\n");
        g_clear_object(&up_proc);
    }
}

static void launch_async_command(const char *command) {
    if (up_proc) {
        g_cancellable_cancel(up_cancel);
        g_clear_object(&up_proc);
        g_clear_object(&up_cancel);
    }
    
    gtk_text_buffer_set_text(out_buffer, "", -1);
    append_output(g_strdup_printf("$ %s\n", command));
    gtk_widget_set_sensitive(btn_check, FALSE);
    gtk_widget_set_sensitive(btn_update, FALSE);

    up_cancel = g_cancellable_new();
    g_autoptr(GError) err = NULL;
    
    // We run it via shell
    const char *argv[] = {"sh", "-c", command, NULL};
    up_proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE, &err);
    
    if (up_proc) {
        GInputStream *stdout_stream = g_subprocess_get_stdout_pipe(up_proc);
        GDataInputStream *data_stream = g_data_input_stream_new(stdout_stream);
        g_data_input_stream_read_line_async(data_stream, G_PRIORITY_DEFAULT, up_cancel, on_read_line, NULL);
        g_object_unref(data_stream);
    } else {
        append_output("Failed to start process.\n");
        gtk_widget_set_sensitive(btn_check, TRUE);
        gtk_widget_set_sensitive(btn_update, TRUE);
    }
}

static void on_check_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    // cd to omarchy settings and git fetch
    g_autofree char *cmd = g_strdup_printf("cd %s && git fetch origin && git remote -v && git status",
        "/home/abyandimas/Development/Settings/hyprland-settings/omarchy-settings");
    launch_async_command(cmd);
}

static void on_update_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    // Either git pull natively or run omarchy-update wrapper script
    // We'll run omarchy-update script
    launch_async_command("if command -v omarchy-update >/dev/null; then omarchy-update; else cd /home/abyandimas/Development/Settings/hyprland-settings/omarchy-settings && git pull origin main && ninja -C build && sudo ninja -C build install; fi");
}

GtkWidget *page_update_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Update");
    adw_preferences_page_set_icon_name(page, "system-software-update-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(grp, "Keep Omarchy core modules and your system settings application up to date.");
    adw_preferences_page_add(page, grp);

    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "System Updates &amp; Git Repositories");
    
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    
    btn_check = gtk_button_new_with_label("Check Output");
    gtk_widget_set_valign(btn_check, GTK_ALIGN_CENTER);
    g_signal_connect(btn_check, "clicked", G_CALLBACK(on_check_clicked), NULL);
    gtk_box_append(GTK_BOX(btn_box), btn_check);

    btn_update = gtk_button_new_with_label("Update System");
    gtk_widget_add_css_class(btn_update, "suggested-action");
    gtk_widget_set_valign(btn_update, GTK_ALIGN_CENTER);
    g_signal_connect(btn_update, "clicked", G_CALLBACK(on_update_clicked), NULL);
    gtk_box_append(GTK_BOX(btn_box), btn_update);

    adw_action_row_add_suffix(row, btn_box);
    adw_preferences_group_add(grp, GTK_WIDGET(row));

    // Live terminal window
    AdwPreferencesGroup *term_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(term_grp, "Update Console");
    adw_preferences_page_add(page, term_grp);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_size_request(scroll, -1, 400);

    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 12);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 12);

    out_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(out_buffer, "Ready. Awaiting commands...\n", -1);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    adw_preferences_group_add(term_grp, scroll);

    return overlay;
}
