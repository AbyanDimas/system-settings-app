#include "page-logs.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static GtkSourceBuffer *log_buffer = NULL;
static AdwComboRow *r_log_type = NULL;

static void on_fetch_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GSubprocess *proc = G_SUBPROCESS(source_object);
    (void)user_data;
    
    g_autoptr(GError) err = NULL;
    g_autofree char *stdout_buf = NULL;
    g_autofree char *stderr_buf = NULL;
    
    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, &stderr_buf, &err);
    
    if (stdout_buf && log_buffer) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(log_buffer), stdout_buf, -1);
    } else if (stderr_buf && log_buffer) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(log_buffer), stderr_buf, -1);
    }
}

static void fetch_logs(const char *cmd) {
    if (log_buffer) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(log_buffer), "Fetching...", -1);
    }
    
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, &err);
    if (proc) {
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_fetch_done, NULL);
    }
}

static void on_refresh_logs(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    guint idx = adw_combo_row_get_selected(r_log_type);
    
    if (idx == 0) {
        fetch_logs("journalctl -b -n 500 --no-pager");
    } else if (idx == 1) {
        fetch_logs("journalctl -p 3 -xb -n 500 --no-pager");
    } else if (idx == 2) {
        fetch_logs("journalctl -k -b -n 500 --no-pager");
    } else if (idx == 3) {
        fetch_logs("grep -i 'installed\\|upgraded\\|removed' /var/log/pacman.log | tail -n 500");
    } else if (idx == 4) {
        fetch_logs("pkexec tail -n 500 /var/log/auth.log || journalctl -t sshd -n 500 --no-pager");
    }
}

static void on_clear_logs(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *cmd = g_strdup("pkexec journalctl --vacuum-time=1d");
    subprocess_run_async(cmd, NULL, NULL);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Vacuumed journal logs older than 1 day"));
    }
}

GtkWidget *page_logs_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Logs");
    adw_preferences_page_set_icon_name(page, "format-text-strikethrough-symbolic"); // fallback document icon
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(ctrl_group, "Query Logs");
    adw_preferences_page_add(page, ctrl_group);

    GtkStringList *lm = gtk_string_list_new(NULL);
    gtk_string_list_append(lm, "System Journal (Current Boot)");
    gtk_string_list_append(lm, "Critical Errors & Warnings");
    gtk_string_list_append(lm, "Kernel Ring Buffer (dmesg)");
    gtk_string_list_append(lm, "Package Manager (Pacman)");
    gtk_string_list_append(lm, "Authentication Details");

    r_log_type = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_log_type), "Log Source");
    adw_combo_row_set_model(r_log_type, G_LIST_MODEL(lm));
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(r_log_type));

    AdwActionRow *ac_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_row), "Actions");
    
    GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *vac_b = gtk_button_new_with_label("Vacuum Journal");
    gtk_widget_add_css_class(vac_b, "destructive-action");
    gtk_widget_set_valign(vac_b, GTK_ALIGN_CENTER);
    g_signal_connect(vac_b, "clicked", G_CALLBACK(on_clear_logs), NULL);
    gtk_box_append(GTK_BOX(bb), vac_b);

    GtkWidget *fetch_b = gtk_button_new_with_label("Fetch Logs");
    gtk_widget_add_css_class(fetch_b, "suggested-action");
    gtk_widget_set_valign(fetch_b, GTK_ALIGN_CENTER);
    g_signal_connect(fetch_b, "clicked", G_CALLBACK(on_refresh_logs), NULL);
    gtk_box_append(GTK_BOX(bb), fetch_b);

    adw_action_row_add_suffix(ac_row, bb);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(ac_row));

    /* ── Viewer ────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *view_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, view_group);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, -1, 500);

    log_buffer = gtk_source_buffer_new(NULL);
    GtkSourceLanguageManager *slm = gtk_source_language_manager_get_default();
    gtk_source_buffer_set_language(log_buffer, gtk_source_language_manager_get_language(slm, "sh"));

    GtkWidget *tv = gtk_source_view_new_with_buffer(log_buffer);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(tv), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 6);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    adw_preferences_group_add(view_group, scroll);

    // Initial fetch
    on_refresh_logs(NULL, NULL);

    return overlay;
}
