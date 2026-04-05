#include "page-accessibility.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

static void on_high_contrast_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = gtk_switch_get_active(sw);
    g_autoptr(GError) err = NULL;
    if (active) {
        subprocess_run_sync("gsettings set org.gnome.desktop.interface gtk-theme 'HighContrast'", &err);
    } else {
        subprocess_run_sync("gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita'", &err);
    }
}

static void on_cursor_size_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("kitty --title 'Cursor Size' bash -c 'size=$(gum input --placeholder \"e.g. 24, 32, 48\"); gsettings set org.gnome.desktop.interface cursor-size \"$size\"; read -p \"Press enter...\"'", &err);
}

GtkWidget *page_accessibility_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Accessibility");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-accessibility-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Visual Assists");
    adw_preferences_page_add(page, group);

    AdwActionRow *hc_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(hc_row), "High Contrast");
    adw_action_row_set_subtitle(hc_row, "Enable GTK High Contrast theme");
    GtkWidget *hc_switch = gtk_switch_new();
    gtk_widget_set_valign(hc_switch, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(hc_row, hc_switch);
    g_signal_connect(hc_switch, "notify::active", G_CALLBACK(on_high_contrast_toggled), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(hc_row));

    AdwActionRow *cursor_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cursor_row), "Custom Cursor Size");
    adw_action_row_set_subtitle(cursor_row, "Set larger cursor size for visibility");
    GtkWidget *cursor_btn = gtk_button_new_with_label("Set Size");
    gtk_widget_set_valign(cursor_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(cursor_row, cursor_btn);
    g_signal_connect(cursor_btn, "clicked", G_CALLBACK(on_cursor_size_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(cursor_row));

    return GTK_WIDGET(page);
}
