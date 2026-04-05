#include "page-users.h"
#include "../utils/subprocess.h"
#include <gtk/gtk.h>
#include <adwaita.h>

static void do_add_user(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    AdwEntryRow *uname_row = g_object_get_data(G_OBJECT(btn), "uname_row");
    AdwPasswordEntryRow *pass_row = g_object_get_data(G_OBJECT(btn), "pass_row");

    const char *uname = gtk_editable_get_text(GTK_EDITABLE(uname_row));
    const char *pass = gtk_editable_get_text(GTK_EDITABLE(pass_row));

    if (strlen(uname) > 0 && strlen(pass) > 0) {
        g_autofree char *cmd = g_strdup_printf("pkexec bash -c \"useradd -m -s /bin/bash '%s' && echo '%s:%s' | chpasswd\"", uname, uname, pass);
        subprocess_run_async(cmd, NULL, NULL);
    }
    adw_dialog_close(dialog);
}

static void on_add_user_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Add New User");
    adw_dialog_set_content_width(dialog, 360);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);

    AdwEntryRow *uname_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(uname_row), "Username");

    AdwPasswordEntryRow *pass_row = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pass_row), "Password");

    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(uname_row));
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(pass_row));

    GtkWidget *apply_btn = gtk_button_new_with_label("Create User");
    gtk_widget_add_css_class(apply_btn, "suggested-action");
    gtk_widget_set_margin_top(apply_btn, 12);
    
    g_object_set_data(G_OBJECT(apply_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(apply_btn), "uname_row", uname_row);
    g_object_set_data(G_OBJECT(apply_btn), "pass_row", pass_row);
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(do_add_user), NULL);

    gtk_box_append(GTK_BOX(vbox), apply_btn);
    adw_dialog_set_child(dialog, vbox);
    
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

static void do_change_pass(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    AdwPasswordEntryRow *pass_row = g_object_get_data(G_OBJECT(btn), "pass_row");

    const char *pass = gtk_editable_get_text(GTK_EDITABLE(pass_row));
    const char *uname = g_get_user_name();

    if (strlen(pass) > 0) {
        g_autofree char *cmd = g_strdup_printf("pkexec bash -c \"echo '%s:%s' | chpasswd\"", uname, pass);
        subprocess_run_async(cmd, NULL, NULL);
    }
    adw_dialog_close(dialog);
}

static void on_change_pass_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, "Change Password");
    adw_dialog_set_content_width(dialog, 360);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);

    AdwPasswordEntryRow *pass_row = ADW_PASSWORD_ENTRY_ROW(adw_password_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pass_row), "New Password");

    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(pass_row));

    GtkWidget *apply_btn = gtk_button_new_with_label("Update Password");
    gtk_widget_add_css_class(apply_btn, "suggested-action");
    gtk_widget_set_margin_top(apply_btn, 12);
    
    g_object_set_data(G_OBJECT(apply_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(apply_btn), "pass_row", pass_row);
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(do_change_pass), NULL);

    gtk_box_append(GTK_BOX(vbox), apply_btn);
    adw_dialog_set_child(dialog, vbox);
    
    adw_dialog_present(dialog, GTK_WIDGET(btn));
}

static void on_manage_groups_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    const char *uname = g_get_user_name();
    g_autofree char *cmd = g_strdup_printf("pkexec usermod -aG wheel,video,audio,input,docker %s", uname);
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_users_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Users & Accounts");
    adw_preferences_page_set_icon_name(page, "system-users-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "User Management");
    adw_preferences_group_set_description(group, "Manage system user accounts, passwords, and groups.");
    adw_preferences_page_add(page, group);

    AdwActionRow *add_user_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_user_row), "Add New User");
    adw_action_row_set_subtitle(add_user_row, "Create a new user account on this system");
    GtkWidget *add_btn = gtk_button_new_with_label("Add User");
    gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(add_btn, "suggested-action");
    adw_action_row_add_suffix(add_user_row, add_btn);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_user_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(add_user_row));

    AdwActionRow *pass_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pass_row), "Change Password");
    adw_action_row_set_subtitle(pass_row, "Update your current account password");
    GtkWidget *pass_btn = gtk_button_new_with_label("Change");
    gtk_widget_set_valign(pass_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(pass_row, pass_btn);
    g_signal_connect(pass_btn, "clicked", G_CALLBACK(on_change_pass_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(pass_row));

    AdwActionRow *groups_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(groups_row), "Add to Essential Groups");
    adw_action_row_set_subtitle(groups_row, "Adds user to wheel, video, audio, input, docker");
    GtkWidget *groups_btn = gtk_button_new_with_label("Manage");
    gtk_widget_set_valign(groups_btn, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(groups_row, groups_btn);
    g_signal_connect(groups_btn, "clicked", G_CALLBACK(on_manage_groups_clicked), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(groups_row));

    return GTK_WIDGET(page);
}
