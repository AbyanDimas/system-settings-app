#include "page-encryption.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void on_backup_header(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *pkname = user_data;
    if (!pkname) return;

    g_autofree char *bak_path = g_build_filename(g_get_home_dir(), g_strdup_printf("luks-header-%s.bak", pkname), NULL);
    g_autofree char *cmd = g_strdup_printf("pkexec cryptsetup luksHeaderBackup /dev/%s --header-backup-file \"%s\"", pkname, bak_path);
    
    g_autoptr(GError) err = NULL;
    char *out = subprocess_run_sync(cmd, &err);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            g_autofree char *msg = g_strdup_printf("Header backed up to %s", bak_path);
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
    g_free(out);
}

static void on_manage_slots(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *pkname = user_data;
    if (!pkname) return;

    g_autofree char *cmd = g_strdup_printf("alacritty -t 'LUKS Key Management' -e bash -c 'sudo cryptsetup luksDump /dev/%s; echo \"\"; echo \"To add key: sudo cryptsetup luksAddKey /dev/%s\"; echo \"To remove key: sudo cryptsetup luksRemoveKey /dev/%s\"; read -p \"Press Enter to exit...\"'", pkname, pkname, pkname);
    
    g_autoptr(GError) err = NULL;
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_encryption_new(void) {
    if (!g_find_program_in_path("cryptsetup")) {
        return omarchy_make_unavailable_page("Disk Encryption", "sudo pacman -S cryptsetup", "drive-harddisk-system-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Disk Encryption");
    adw_preferences_page_set_icon_name(page, "drive-harddisk-system-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(grp, "LUKS Encrypted Volumes");
    adw_preferences_group_set_description(grp, "Manage block device encryption keys and headers.");
    adw_preferences_page_add(page, grp);

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("lsblk -r -o NAME,TYPE,PKNAME,MOUNTPOINT", &err);
    gboolean found = FALSE;

    if (out) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i = 1; lines[i] != NULL; i++) { // skip header
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }

            g_auto(GStrv) toks = g_strsplit(line, " ", 4);
            int t_cnt = g_strv_length(toks);
            
            // NAME TYPE PKNAME MOUNTPOINT
            if (t_cnt >= 3 && strcmp(toks[1], "crypt") == 0) {
                found = TRUE;
                const char *name = toks[0];
                const char *pkname = toks[2];
                const char *mnt = (t_cnt == 4) ? toks[3] : "Not mounted";

                AdwExpanderRow *row = ADW_EXPANDER_ROW(adw_expander_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_strdup_printf("Volume: /dev/mapper/%s", name));
                adw_expander_row_set_subtitle(row, g_strdup_printf("Physical device: /dev/%s  |  Mount: %s", pkname, mnt));

                AdwActionRow *sr1 = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr1), "Backup LUKS Header");
                adw_action_row_set_subtitle(sr1, "Safeguard against header corruption (very important!)");
                GtkWidget *b1 = gtk_button_new_with_label("Backup");
                gtk_widget_set_valign(b1, GTK_ALIGN_CENTER);
                g_signal_connect_data(b1, "clicked", G_CALLBACK(on_backup_header), g_strdup(pkname), (GClosureNotify)free_g_free, 0);
                adw_action_row_add_suffix(sr1, b1);
                adw_expander_row_add_row(row, GTK_WIDGET(sr1));

                AdwActionRow *sr2 = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr2), "Manage Key Slots");
                adw_action_row_set_subtitle(sr2, "Add or remove encryption passphrases");
                GtkWidget *b2 = gtk_button_new_with_label("Manage");
                gtk_widget_set_valign(b2, GTK_ALIGN_CENTER);
                g_signal_connect_data(b2, "clicked", G_CALLBACK(on_manage_slots), g_strdup(pkname), (GClosureNotify)free_g_free, 0);
                adw_action_row_add_suffix(sr2, b2);
                adw_expander_row_add_row(row, GTK_WIDGET(sr2));

                adw_preferences_group_add(grp, GTK_WIDGET(row));
            }
            g_free(line);
        }
    }

    if (!found) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No LUKS encrypted volumes detected.");
        adw_preferences_group_add(grp, GTK_WIDGET(row));
    }

    return overlay;
}
