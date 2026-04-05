#include "page-printers.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

static void on_open_cups_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("xdg-open http://localhost:631", &err);
}

static void on_start_cups_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("pkexec systemctl enable --now cups.service", &err);
}

GtkWidget *page_printers_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Printers & Scanners");
    adw_preferences_page_set_icon_name(page, "printer-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "CUPS Printing System");
    adw_preferences_group_set_description(group, "Manage network and local printers via CUPS web interface.");
    adw_preferences_page_add(page, group);

    AdwActionRow *start_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(start_row), "Start CUPS Service");
    adw_action_row_set_subtitle(start_row, "Enable and start the cups systemd service");
    GtkWidget *start_btn = gtk_button_new_with_label("Enable Service");
    gtk_widget_set_valign(start_btn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(start_btn, "suggested-action");
    adw_action_row_add_suffix(start_row, start_btn);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_cups_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(start_row));

    AdwActionRow *web_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(web_row), "Open Printer Dashboard");
    adw_action_row_set_subtitle(web_row, "Launch Localhost CUPS Web GUI");
    GtkWidget *web_btn = gtk_button_new_with_label("Open Website");
    gtk_widget_set_valign(web_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(web_row, web_btn);
    g_signal_connect(web_btn, "clicked", G_CALLBACK(on_open_cups_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(web_row));

    return GTK_WIDGET(page);
}
