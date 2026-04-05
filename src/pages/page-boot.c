#include "page-boot.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

#include <gtksourceview/gtksource.h>

static void on_boot_save_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    GtkSourceBuffer *buffer = g_object_get_data(G_OBJECT(btn), "buffer");
    const char *target_file = g_object_get_data(G_OBJECT(btn), "target_file");
    const char *post_cmd = g_object_get_data(G_OBJECT(btn), "post_cmd");
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
    g_autofree char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
    
    g_autofree char *tmp_path = g_strdup_printf("/tmp/omarchy_boot_%d.tmp", g_random_int());
    g_file_set_contents(tmp_path, text, -1, NULL);
    
    g_autofree char *cmd = NULL;
    if (post_cmd && strlen(post_cmd) > 0) {
        cmd = g_strdup_printf("pkexec bash -c \"cp %s %s && rm %s && %s\"", tmp_path, target_file, tmp_path, post_cmd);
    } else {
        cmd = g_strdup_printf("pkexec bash -c \"cp %s %s && rm %s\"", tmp_path, target_file, tmp_path);
    }
    
    subprocess_run_async(cmd, NULL, NULL);
    adw_dialog_close(dialog);
}

static void show_boot_editor_dialog(GtkWidget *parent, const char *title, const char *target_file, const char *post_cmd) {
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, title);
    adw_dialog_set_content_width(dialog, 600);
    adw_dialog_set_content_height(dialog, 500);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkSourceView *view = GTK_SOURCE_VIEW(gtk_source_view_new());
    GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    
    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_mgr, "ini");
    if (!lang) lang = gtk_source_language_manager_get_language(lang_mgr, "sh");
    if (lang) gtk_source_buffer_set_language(buffer, lang);

    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_tab_width(view, 4);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);

    g_autofree char *contents = NULL;
    if (g_file_get_contents(target_file, &contents, NULL, NULL) && contents) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, -1);
    } else {
        g_autofree char *err_msg = g_strdup_printf("# Failed to read %s\n", target_file);
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), err_msg, -1);
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
    g_object_set_data_full(G_OBJECT(save_btn), "target_file", g_strdup(target_file), g_free);
    if (post_cmd) g_object_set_data_full(G_OBJECT(save_btn), "post_cmd", g_strdup(post_cmd), g_free);
    
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_boot_save_clicked), NULL);

    gtk_box_append(GTK_BOX(vbox), save_btn);

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, parent);
}

static void on_edit_grub_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    show_boot_editor_dialog(GTK_WIDGET(btn), "Edit GRUB Configuration", "/etc/default/grub", "grub-mkconfig -o /boot/grub/grub.cfg");
}

static void on_edit_systemdboot_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    show_boot_editor_dialog(GTK_WIDGET(btn), "Edit systemd-boot Configuration", "/boot/loader/loader.conf", NULL);
}

static void on_change_kernel_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("sh -c 'pacman -Q | grep -E \"^linux \"'", &err);
    
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Installed Kernels");
    adw_dialog_set_content_width(dialog, 360);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(group, "Use package manager (pacman) to install or remove. E.g: sudo pacman -S linux-zen");
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(group));
    
    if (out) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            if (strlen(lines[i]) < 4) continue;
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), lines[i]);
            adw_action_row_add_prefix(row, gtk_image_new_from_icon_name("system-software-install-symbolic"));
            adw_preferences_group_add(group, GTK_WIDGET(row));
        }
    } else {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No linux kernels found?");
        adw_preferences_group_add(group, GTK_WIDGET(row));
    }

    adw_dialog_set_child(dialog, vbox);
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

GtkWidget *page_boot_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Boot & Kernel");
    adw_preferences_page_set_icon_name(page, "drive-harddisk-system-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Bootloader Specifics");
    adw_preferences_page_add(page, group);

    AdwActionRow *grub_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(grub_row), "GRUB Configuration");
    adw_action_row_set_subtitle(grub_row, "Edit /etc/default/grub and update-grub");
    GtkWidget *grub_btn = gtk_button_new_with_label("Configure GRUB");
    gtk_widget_set_valign(grub_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(grub_row, grub_btn);
    g_signal_connect(grub_btn, "clicked", G_CALLBACK(on_edit_grub_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(grub_row));

    AdwActionRow *sdboot_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sdboot_row), "systemd-boot Configuration");
    adw_action_row_set_subtitle(sdboot_row, "Edit /boot/loader/loader.conf");
    GtkWidget *sdboot_btn = gtk_button_new_with_label("Configure systemd-boot");
    gtk_widget_set_valign(sdboot_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(sdboot_row, sdboot_btn);
    g_signal_connect(sdboot_btn, "clicked", G_CALLBACK(on_edit_systemdboot_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(sdboot_row));

    AdwPreferencesGroup *kernel_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(kernel_group, "Linux Kernel");
    adw_preferences_page_add(page, kernel_group);

    AdwActionRow *kernel_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(kernel_row), "Manage Kernel Packages");
    adw_action_row_set_subtitle(kernel_row, "linux, linux-lts, linux-zen");
    GtkWidget *kernel_btn = gtk_button_new_with_label("View Kernels");
    gtk_widget_set_valign(kernel_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(kernel_row, kernel_btn);
    g_signal_connect(kernel_btn, "clicked", G_CALLBACK(on_change_kernel_clicked), NULL);
    adw_preferences_group_add(kernel_group, GTK_WIDGET(kernel_row));

    return GTK_WIDGET(page);
}
