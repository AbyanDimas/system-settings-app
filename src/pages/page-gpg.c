#include "page-gpg.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *keys_group = NULL;
static GtkWidget *list_box = NULL;

static void refresh_gpg(void);

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void on_copy_key(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *key_id = user_data;
    GdkClipboard *cb = gdk_display_get_clipboard(gdk_display_get_default());
    gdk_clipboard_set_text(cb, key_id);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Key ID copied to clipboard"));
    }
}

static void on_export_key(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *key_id = user_data;
    g_autofree char *path = g_build_filename(g_get_home_dir(), "Downloads", g_strdup_printf("GPG_Public_Key_%s.asc", key_id), NULL);
    g_autofree char *cmd = g_strdup_printf("gpg --armor --export %s > \"%s\"", key_id, path);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            g_autofree char *msg = g_strdup_printf("Exported to %s", path);
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
}

static void on_delete_key(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *key_id = user_data;
    g_autofree char *cmd = g_strdup_printf("alacritty -e gpg --delete-secret-and-public-key %s", key_id);
    g_autoptr(GError) err = NULL;
    subprocess_run_async(cmd, NULL, NULL);
    // Might need refresh later, but deletion is async interactive
}

static void refresh_gpg(void) {
    if (!list_box) return;
    
    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("gpg --list-keys --with-colons", &err);
    if (!out) return;

    g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
    char *current_pub_id = NULL;

    for (int i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];
        if (g_str_has_prefix(line, "pub:")) {
            g_auto(GStrv) cols = g_strsplit(line, ":", -1);
            if (g_strv_length(cols) >= 10) {
                g_free(current_pub_id);
                current_pub_id = g_strdup(cols[4]); // Key ID
            }
        } else if (g_str_has_prefix(line, "uid:") && current_pub_id) {
            g_auto(GStrv) cols = g_strsplit(line, ":", -1);
            if (g_strv_length(cols) >= 10) {
                char *uid = g_strdup(cols[9]); // UID string
AdwExpanderRow *row = ADW_EXPANDER_ROW(adw_expander_row_new());
adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), uid);
adw_expander_row_set_subtitle(row, g_strdup_printf("Key ID: %s", current_pub_id));

GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
gtk_widget_set_valign(bb, GTK_ALIGN_CENTER);

GtkWidget *bc = gtk_button_new_from_icon_name("edit-copy-symbolic");
gtk_widget_set_valign(bc, GTK_ALIGN_CENTER);
gtk_widget_set_tooltip_text(bc, "Copy Key ID");
g_signal_connect_data(bc, "clicked", G_CALLBACK(on_copy_key), g_strdup(current_pub_id), (GClosureNotify)free_g_free, 0);
gtk_box_append(GTK_BOX(bb), bc);
adw_expander_row_add_suffix(row, bb);

                // Export
                AdwActionRow *sr1 = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr1), "Export Public Key (.asc)");
                GtkWidget *b1 = gtk_button_new_with_label("Export to Downloads");
                gtk_widget_set_valign(b1, GTK_ALIGN_CENTER);
                g_signal_connect_data(b1, "clicked", G_CALLBACK(on_export_key), g_strdup(current_pub_id), (GClosureNotify)free_g_free, 0);
                adw_action_row_add_suffix(sr1, b1);
                adw_expander_row_add_row(row, GTK_WIDGET(sr1));

                // Delete
                AdwActionRow *sr2 = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr2), "Delete Certificate");
                GtkWidget *b2 = gtk_button_new_from_icon_name("user-trash-symbolic");
                gtk_widget_add_css_class(b2, "destructive-action");
                gtk_widget_set_valign(b2, GTK_ALIGN_CENTER);
                g_signal_connect_data(b2, "clicked", G_CALLBACK(on_delete_key), g_strdup(current_pub_id), (GClosureNotify)free_g_free, 0);
                adw_action_row_add_suffix(sr2, b2);
                adw_expander_row_add_row(row, GTK_WIDGET(sr2));

                gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
                g_free(uid);
            }
        }
    }
    g_free(current_pub_id);
}

static void on_gen_key(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    subprocess_run_async("alacritty -t 'GPG Key Generation' -e gpg --full-generate-key", NULL, NULL);
}

static void on_file_opened(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    (void)user_data;
    g_autoptr(GError) err = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, res, &err);
    if (file) {
        g_autofree char *path = g_file_get_path(file);
        g_autofree char *cmd = g_strdup_printf("alacritty -e gpg --import \"%s\"", path);
        subprocess_run_async(cmd, NULL, NULL);
        g_object_unref(file);
    }
}

static void on_import_key(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select GPG Public Key to Import");
    if (page_widget) {
        GtkRoot *root = gtk_widget_get_root(page_widget);
        gtk_file_dialog_open(dialog, GTK_WINDOW(root), NULL, on_file_opened, NULL);
    }
}

GtkWidget *page_gpg_new(void) {
    if (!g_find_program_in_path("gpg")) {
        return omarchy_make_unavailable_page("GPG & Passwords", "sudo pacman -S gnupg", "emblem-readonly-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "GPG & Passwords");
    adw_preferences_page_set_icon_name(page, "emblem-readonly-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *hdr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(hdr_group, "Keyring Actions");
    adw_preferences_page_add(page, hdr_group);

    AdwActionRow *gen_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gen_row), "Generate New GPG Key");
    GtkWidget *gen_btn = gtk_button_new_with_label("Generate");
    gtk_widget_add_css_class(gen_btn, "suggested-action");
    gtk_widget_set_valign(gen_btn, GTK_ALIGN_CENTER);
    g_signal_connect(gen_btn, "clicked", G_CALLBACK(on_gen_key), NULL);
    adw_action_row_add_suffix(gen_row, gen_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(gen_row));

    AdwActionRow *imp_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(imp_row), "Import Key from File");
    GtkWidget *imp_btn = gtk_button_new_with_label("Import (.asc/.gpg)");
    gtk_widget_set_valign(imp_btn, GTK_ALIGN_CENTER);
    g_signal_connect(imp_btn, "clicked", G_CALLBACK(on_import_key), NULL);
    adw_action_row_add_suffix(imp_row, imp_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(imp_row));

    AdwActionRow *ref_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ref_row), "Refresh Keyring");
    GtkWidget *ref_btn = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_set_valign(ref_btn, GTK_ALIGN_CENTER);
    g_signal_connect(ref_btn, "clicked", G_CALLBACK(refresh_gpg), NULL);
    adw_action_row_add_suffix(ref_row, ref_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(ref_row));

    keys_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(keys_group, "Public Keyring");
    adw_preferences_page_add(page, keys_group);

    list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(keys_group, list_box);

    refresh_gpg();

    return overlay;
}
