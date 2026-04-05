#include "page-storage.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void on_open_tool(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *tool = user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_async(tool, NULL, NULL);
}

typedef struct {
    char *name;
    char *size;
    char *type;
    char *mount;
} BlockDev;

GtkWidget *page_storage_new(void) {
    if (!g_find_program_in_path("lsblk")) {
        return omarchy_make_unavailable_page("Storage", "sudo pacman -S util-linux", "drive-harddisk-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Storage");
    adw_preferences_page_set_icon_name(page, "drive-harddisk-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── Disks & Volumes ───────────────────────────────────────────────────── */
    AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(grp, "Local Volumes");
    adw_preferences_page_add(page, grp);

    g_autofree char *out = subprocess_run_sync("lsblk -r -o NAME,SIZE,TYPE,MOUNTPOINT", &err);
    if (out) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i = 1; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }

            g_auto(GStrv) toks = g_strsplit(line, " ", 4);
            int t_cnt = g_strv_length(toks);
            if (t_cnt >= 3) {
                const char *name = toks[0];
                const char *size = toks[1];
                const char *type = toks[2];
                const char *mnt = (t_cnt == 4) ? toks[3] : NULL;

                // Only show crypt, part, loop optionally, lvm, btrfs
                if (strcmp(type, "disk") == 0 || strcmp(type, "crypt") == 0 || strcmp(type, "part") == 0 || strcmp(type, "lvm") == 0) {
                    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_strdup_printf("/dev/%s", name));
                    
                    GString *sub = g_string_new("");
                    g_string_append_printf(sub, "Size: %s  ·  Type: %s", size, type);

                    if (mnt && strlen(mnt) > 0) {
                        g_string_append_printf(sub, "  ·  Mounted on: %s", mnt);
                        
                        // Check usage
                        g_autofree char *df_cmd = g_strdup_printf("df -h \"%s\" | tail -n 1", mnt);
                        g_autofree char *df_out = subprocess_run_sync(df_cmd, &err);
                        if (df_out) {
                            g_auto(GStrv) d_toks = g_strsplit_set(g_strstrip(df_out), " \t", -1);
                            GPtrArray *real = g_ptr_array_new();
                            for (int k = 0; d_toks[k]; k++) {
                                if (strlen(d_toks[k]) > 0) g_ptr_array_add(real, d_toks[k]);
                            }
                            if (real->len >= 5) { // Size Used Avail Use%
                                const char *used = g_ptr_array_index(real, 2);
                                const char *avail = g_ptr_array_index(real, 3);
                                const char *pct_str = g_ptr_array_index(real, 4);
                                
                                g_string_append_printf(sub, "  (Used %s, Free %s)", used, avail);

                                // Add a miniature progress bar suffix
                                int pct = atoi(pct_str);
                                GtkWidget *pb = gtk_progress_bar_new();
                                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), (double)pct / 100.0);
                                gtk_widget_set_size_request(pb, 100, -1);
                                gtk_widget_set_valign(pb, GTK_ALIGN_CENTER);
                                adw_action_row_add_suffix(row, pb);
                            }
                            g_ptr_array_unref(real);
                        }
                    } else {
                        g_string_append(sub, "  ·  Unmounted");
                    }
                    
                    adw_action_row_set_subtitle(row, sub->str);
                    adw_preferences_group_add(grp, GTK_WIDGET(row));
                    g_string_free(sub, TRUE);
                }
            }
            g_free(line);
        }
    }

    /* ── Storage Tools ─────────────────────────────────────────────────────── */
    AdwPreferencesGroup *tools_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(tools_group, "Storage Tools");
    adw_preferences_page_add(page, tools_group);

    if (g_find_program_in_path("baobab")) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Disk Usage Analyzer");
        adw_action_row_set_subtitle(row, "Visualize folder sizes inside Baobab");
        GtkWidget *ic = gtk_image_new_from_icon_name("baobab");
        adw_action_row_add_prefix(row, ic);

        GtkWidget *btn = gtk_button_new_from_icon_name("external-link-symbolic");
        gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
        g_signal_connect_data(btn, "clicked", G_CALLBACK(on_open_tool), g_strdup("baobab"), free_g_free, 0);
        adw_action_row_add_suffix(row, btn);
        adw_preferences_group_add(tools_group, GTK_WIDGET(row));
    }

    if (g_find_program_in_path("gnome-disks")) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Manage Disks & Drives");
        adw_action_row_set_subtitle(row, "Format, benchmark, and edit mount options");
        GtkWidget *ic = gtk_image_new_from_icon_name("gnome-disks");
        adw_action_row_add_prefix(row, ic);

        GtkWidget *btn = gtk_button_new_from_icon_name("external-link-symbolic");
        gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
        g_signal_connect_data(btn, "clicked", G_CALLBACK(on_open_tool), g_strdup("gnome-disks"), free_g_free, 0);
        adw_action_row_add_suffix(row, btn);
        adw_preferences_group_add(tools_group, GTK_WIDGET(row));
    }

    return overlay;
}
