#include "page-snapshots.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *list_group = NULL;
static GtkWidget *list_box = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void refresh_snapshots(void);

static void on_delete_snapper(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *id = user_data;
    g_autofree char *cmd = g_strdup_printf("pkexec snapper delete %s", id);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
    refresh_snapshots();
}

static void on_rollback_snapper(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *id = user_data;
    g_autofree char *cmd = g_strdup_printf("pkexec snapper rollback %s", id);
    g_autoptr(GError) err = NULL;
    char *out = subprocess_run_sync(cmd, &err);
    if (page_widget && out) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Rollback executed. Please reboot your system."));
    }
    g_free(out);
}

static void on_create_snapper(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("pkexec snapper create -c timeline -d \"System Manual Snapshot\"", &err);
    refresh_snapshots();
}

static void on_delete_timeshift(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *name = user_data;
    g_autofree char *cmd = g_strdup_printf("pkexec timeshift --delete --snapshot '%s'", name);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
    refresh_snapshots();
}

static void on_restore_timeshift(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *name = user_data;
    // Real timeshift restore usually requires interactive/terminal. 
    // GUI mode is `pkexec timeshift-gtk` but we can try CLI. 
    // Timeshift restore without terminal is complicated, maybe better to launch terminal.
    g_autofree char *cmd = g_strdup_printf("alacritty -e sudo timeshift --restore --snapshot '%s' &", name);
    g_autoptr(GError) err = NULL;
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_create_timeshift(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    // Timeshift creation is async to avoid blocking UI for 5 minutes
    subprocess_run_async("pkexec timeshift --create --comments \"System Manual Snapshot\"", NULL, NULL);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Timeshift snapshot creation started in background..."));
    }
}

static void refresh_snapshots(void) {
    if (!list_box) return;
    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    g_autoptr(GError) err = NULL;

    if (g_find_program_in_path("snapper")) {
        // Output format: number,date,description using CSV
        char *out = subprocess_run_sync("pkexec snapper --csv list --columns number,date,description", &err);
        if (out) {
            g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
            for (int i = 1; lines[i] != NULL; i++) { // skip header
                if (strlen(lines[i]) < 2) continue;
                g_auto(GStrv) cols = g_strsplit(lines[i], ",", 3);
                if (cols[0] && cols[1] && cols[2]) {
                    if (g_strcmp0(cols[0], "0") == 0) continue; // skip current

                    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                    g_autofree char *title = g_strdup_printf("Snapshot #%s: %s", cols[0], cols[2]);
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
                    adw_action_row_set_subtitle(row, cols[1]);

                    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                    
                    GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
                    gtk_widget_add_css_class(del_btn, "destructive-action");
                    gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
                    g_signal_connect_data(del_btn, "clicked", G_CALLBACK(on_delete_snapper), g_strdup(cols[0]), free_g_free, 0);
                    gtk_box_append(GTK_BOX(btn_box), del_btn);

                    GtkWidget *res_btn = gtk_button_new_with_label("Rollback");
                    gtk_widget_set_valign(res_btn, GTK_ALIGN_CENTER);
                    g_signal_connect_data(res_btn, "clicked", G_CALLBACK(on_rollback_snapper), g_strdup(cols[0]), free_g_free, 0);
                    gtk_box_append(GTK_BOX(btn_box), res_btn);

                    adw_action_row_add_suffix(row, btn_box);
                    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
                }
            }
            g_free(out);
        }
    } else if (g_find_program_in_path("timeshift")) {
        char *out = subprocess_run_sync("pkexec timeshift --list", &err);
        if (out) {
            g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
            for (int i = 0; lines[i] != NULL; i++) {
                char *line = g_strstrip(g_strdup(lines[i]));
                // Num     Name                 Tags  Description  
                // 0    > 2025-01-01_12-00-00  O     First Backup
                if (strlen(line) > 10 && g_ascii_isdigit(line[0])) {
                    char *name_start = strchr(line, '>');
                    if (name_start) {
                        name_start++;
                        while(*name_start == ' ') name_start++;
                        char *name_end = strchr(name_start, ' ');
                        if (name_end) {
                            char *snap_name = g_strndup(name_start, name_end - name_start);
                            char *desc = g_strstrip(g_strdup(name_end));

                            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), snap_name);
                            adw_action_row_set_subtitle(row, desc);

                            GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                            
                            GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
                            gtk_widget_add_css_class(del_btn, "destructive-action");
                            gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
                            g_signal_connect_data(del_btn, "clicked", G_CALLBACK(on_delete_timeshift), g_strdup(snap_name), free_g_free, 0);
                            gtk_box_append(GTK_BOX(btn_box), del_btn);

                            GtkWidget *res_btn = gtk_button_new_with_label("Restore");
                            gtk_widget_set_valign(res_btn, GTK_ALIGN_CENTER);
                            g_signal_connect_data(res_btn, "clicked", G_CALLBACK(on_restore_timeshift), g_strdup(snap_name), free_g_free, 0);
                            gtk_box_append(GTK_BOX(btn_box), res_btn);

                            adw_action_row_add_suffix(row, btn_box);
                            gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
                            
                            g_free(snap_name);
                            g_free(desc);
                        }
                    }
                }
                g_free(line);
            }
            g_free(out);
        }
    }
}

GtkWidget *page_snapshots_new(void) {
    if (!g_find_program_in_path("snapper") && !g_find_program_in_path("timeshift")) {
        return omarchy_make_unavailable_page("System Snapshots", "sudo pacman -S snapper OR timeshift", "drive-harddisk-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Snapshots");
    adw_preferences_page_set_icon_name(page, "drive-harddisk-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *hdr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(hdr_group, g_find_program_in_path("snapper") ? "Snapper Engine" : "Timeshift Engine");
    adw_preferences_page_add(page, hdr_group);

    AdwActionRow *add_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_row), "Create Manual Snapshot");
    GtkWidget *add_btn = gtk_button_new_with_label("Create");
    gtk_widget_add_css_class(add_btn, "suggested-action");
    gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
    if (g_find_program_in_path("snapper")) {
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_create_snapper), NULL);
    } else {
        g_signal_connect(add_btn, "clicked", G_CALLBACK(on_create_timeshift), NULL);
    }
    adw_action_row_add_suffix(add_row, add_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(add_row));

    AdwActionRow *ref_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ref_row), "Refresh List");
    GtkWidget *ref_btn = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_set_valign(ref_btn, GTK_ALIGN_CENTER);
    g_signal_connect(ref_btn, "clicked", G_CALLBACK(refresh_snapshots), NULL);
    adw_action_row_add_suffix(ref_row, ref_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(ref_row));

    list_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(list_group, "Available Snapshots");
    adw_preferences_page_add(page, list_group);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(list_group, list_box);

    refresh_snapshots();

    return overlay;
}
