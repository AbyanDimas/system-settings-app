#include "page-hosts.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *entries_group = NULL;
static GtkWidget *list_box = NULL;
static GtkSourceBuffer *raw_buffer = NULL;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static void refresh_hosts(void);

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_save_raw(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(raw_buffer), &start, &end);
    g_autofree char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(raw_buffer), &start, &end, FALSE);
    
    g_file_set_contents("/tmp/omarchy_hosts_edit", text, -1, NULL);
    
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("pkexec cp /tmp/omarchy_hosts_edit /etc/hosts", &err);
    
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("/etc/hosts saved."));
    }
    refresh_hosts();
}

static void on_install_adblock(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Downloading StevenBlack Hosts list... (might take a moment)"));
    }

    g_autoptr(GError) err = NULL;
    subprocess_run_async("alacritty -t 'AdBlock Hosts Install' -e bash -c 'echo \"Downloading StevenBlack hosts...\" && curl -sS https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts > /tmp/new_hosts && echo \"Applying new hosts file...\" && pkexec cp /tmp/new_hosts /etc/hosts && echo \"Done! Press Enter.\" && read'", NULL, NULL);
}

static void on_remove_adblock(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    // Default arch hosts is just localhost maps
    subprocess_run_sync("pkexec bash -c 'echo \"127.0.0.1 localhost\" > /etc/hosts && echo \"::1 localhost\" >> /etc/hosts'", &err);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Ad-block removed. Hosts reset to default."));
    }
    refresh_hosts();
}

static void on_delete_entry(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *line_exact = user_data;
    // We sed out the exact line
    g_autofree char *escaped = g_regex_escape_string(line_exact, -1);
    g_autofree char *cmd = g_strdup_printf("pkexec sed -i '/^%s$/d' /etc/hosts", escaped);
    
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
    refresh_hosts();
}

static void on_add_entry_done(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_ip = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    AdwEntryRow *r_host = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_ip)));
    
    const char *ip = gtk_editable_get_text(GTK_EDITABLE(r_ip));
    const char *host = gtk_editable_get_text(GTK_EDITABLE(r_host));

    if (strlen(ip) > 0 && strlen(host) > 0) {
        g_autofree char *cmd = g_strdup_printf("pkexec bash -c 'echo \"%s %s\" >> /etc/hosts'", ip, host);
        g_autoptr(GError) err = NULL;
        subprocess_run_sync(cmd, &err);
        refresh_hosts();
    }
    adw_dialog_close(dlg);
}

static void on_add_entry(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Add Host Entry");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);
    
    GtkWidget *lbl = gtk_label_new("Map an IP Address to a Hostname");
    gtk_box_append(GTK_BOX(box), lbl);
    
    AdwEntryRow *r_ip = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_ip), "IP Address (e.g. 192.168.1.5)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_ip));
    
    AdwEntryRow *r_host = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_host), "Hostname (e.g. myserver.local)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_host));
    
    GtkWidget *btn_add = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(btn_add, "suggested-action");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_entry_done), dlg);
    gtk_box_append(GTK_BOX(box), btn_add);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

static void refresh_hosts(void) {
    if (!list_box) return;
    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    g_autofree char *contents = NULL;
    gsize length = 0;
    if (g_file_get_contents("/etc/hosts", &contents, &length, NULL)) {
        if (raw_buffer) {
            gtk_text_buffer_set_text(GTK_TEXT_BUFFER(raw_buffer), contents, length);
        }

        g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
        int custom_count = 0;
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0 || line[0] == '#') { g_free(line); continue; }

            // Skip default localhosts
            if (strstr(line, "localhost") || strstr(line, "broadcasthost") || 
                strcmp(line, "255.255.255.255 broadcasthost") == 0 ||
                strcmp(line, "127.0.0.1") == 0 || strcmp(line, "::1") == 0) {
                g_free(line);
                continue;
            }

            custom_count++;
            
            // Limit UI items if it's an adblock list
            if (custom_count <= 100) {
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                g_auto(GStrv) toks = g_strsplit(line, " ", 2);
                if (toks[0] && toks[1]) {
                    char *host = g_strstrip(g_strdup(toks[1]));
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), host);
                    adw_action_row_set_subtitle(row, toks[0]);
                    g_free(host);
                } else {
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), line);
                }

                GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
                gtk_widget_add_css_class(del_btn, "destructive-action");
                gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
                g_signal_connect_data(del_btn, "clicked", G_CALLBACK(on_delete_entry), g_strdup(lines[i]), (GClosureNotify)free_g_free, 0);
                adw_action_row_add_suffix(row, del_btn);
                
                gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
            }
            g_free(line);
        }

        if (custom_count > 100) {
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_strdup_printf("+ %d more entries normally used for ad-blocking...", custom_count - 100));
            gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
        } else if (custom_count == 0) {
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No custom host entries");
            gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
        }
    }
}

GtkWidget *page_hosts_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Hosts File");
    adw_preferences_page_set_icon_name(page, "network-wired-symbolic"); // fallback icon
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Ad Blocker ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ab_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(ab_group, "System-wide Ad Blocking");
    adw_preferences_group_set_description(ab_group, "Block ads across all apps by routing tracking domains to 0.0.0.0 via /etc/hosts");
    adw_preferences_page_add(page, ab_group);

    AdwActionRow *ab_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ab_row), "StevenBlack Hosts");
    
    GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *ab_btn = gtk_button_new_with_label("Install");
    gtk_widget_add_css_class(ab_btn, "suggested-action");
    gtk_widget_set_valign(ab_btn, GTK_ALIGN_CENTER);
    g_signal_connect(ab_btn, "clicked", G_CALLBACK(on_install_adblock), NULL);
    gtk_box_append(GTK_BOX(bb), ab_btn);

    GtkWidget *rm_btn = gtk_button_new_with_label("Remove");
    gtk_widget_add_css_class(rm_btn, "destructive-action");
    gtk_widget_set_valign(rm_btn, GTK_ALIGN_CENTER);
    g_signal_connect(rm_btn, "clicked", G_CALLBACK(on_remove_adblock), NULL);
    gtk_box_append(GTK_BOX(bb), rm_btn);
    adw_action_row_add_suffix(ab_row, bb);
    adw_preferences_group_add(ab_group, GTK_WIDGET(ab_row));

    /* ── Entries List ──────────────────────────────────────────────────────── */
    entries_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(entries_group, "Custom Entries");
    
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *ref_b = gtk_button_new_from_icon_name("view-refresh-symbolic");
    g_signal_connect(ref_b, "clicked", G_CALLBACK(refresh_hosts), NULL);
    gtk_box_append(GTK_BOX(top_box), ref_b);
    
    GtkWidget *add_b = gtk_button_new_from_icon_name("list-add-symbolic");
    gtk_widget_add_css_class(add_b, "suggested-action");
    g_signal_connect(add_b, "clicked", G_CALLBACK(on_add_entry), NULL);
    gtk_box_append(GTK_BOX(top_box), add_b);
    adw_preferences_group_set_header_suffix(entries_group, top_box);
    adw_preferences_page_add(page, entries_group);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(entries_group, list_box);

    /* ── Raw Editor ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *raw_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(raw_group, "Raw Editor");
    
    GtkWidget *sv_b = gtk_button_new_with_label("Save Raw");
    gtk_widget_add_css_class(sv_b, "suggested-action");
    g_signal_connect(sv_b, "clicked", G_CALLBACK(on_save_raw), NULL);
    adw_preferences_group_set_header_suffix(raw_group, sv_b);
    adw_preferences_page_add(page, raw_group);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, -1, 300);
    raw_buffer = gtk_source_buffer_new(NULL);
    GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
    gtk_source_buffer_set_language(raw_buffer, gtk_source_language_manager_get_language(lm, "sh"));
    
    GtkWidget *tv = gtk_source_view_new_with_buffer(raw_buffer);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(tv), TRUE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    adw_preferences_group_add(raw_group, scroll);

    refresh_hosts();

    return overlay;
}
