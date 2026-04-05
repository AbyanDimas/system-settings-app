#include "page-xcompose.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *sequence;
    char *result;
    char *raw_line;
    AdwActionRow *ui_row;
} XComposeMacro;

static GList *macros = NULL;
static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *list_group = NULL;
static GtkWidget *list_box = NULL;

static void free_macro(XComposeMacro *m) {
    g_free(m->sequence);
    g_free(m->result);
    g_free(m->raw_line);
    g_free(m);
}

static void load_xcompose(void) {
    g_list_free_full(macros, (GDestroyNotify)free_macro);
    macros = NULL;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".XCompose", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) {
        // Create default if missing
        contents = g_strdup("include \"%L\"\n\n");
        g_file_set_contents(path, contents, -1, NULL);
    }

    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        XComposeMacro *m = g_new0(XComposeMacro, 1);
        m->raw_line = g_strdup(line);

        if (g_str_has_prefix(line, "<Multi_key>")) {
            char *colon = strchr(line, ':');
            if (colon) {
                // Parse sequence
                char *seq = g_strndup(line + 11, colon - (line + 11));
                g_strstrip(seq);
                m->sequence = seq;

                // Parse result
                char *res = g_strdup(colon + 1);
                g_strstrip(res);
                // Strip quotes if any
                if (res[0] == '"') {
                    char *endq = strrchr(res, '"');
                    if (endq && endq != res) {
                        *endq = '\0';
                        char *clean = g_strdup(res + 1);
                        g_free(res);
                        res = clean;
                    }
                }
                m->result = res;
            }
        }
        macros = g_list_append(macros, m);
        g_free(line);
    }
}

static void save_xcompose(void) {
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".XCompose", NULL);
    GString *out = g_string_new("");

    for (GList *iter = macros; iter; iter = iter->next) {
        XComposeMacro *m = iter->data;
        if (m->sequence && m->result) {
            g_string_append_printf(out, "<Multi_key> %s : \"%s\"\n", m->sequence, m->result);
        } else {
            g_string_append_printf(out, "%s\n", m->raw_line);
        }
    }

    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(".XCompose saved. Restart apps to apply."));
    }
}
static void on_delete_macro(GtkButton *btn, gpointer user_data) {
    (void)btn;
    XComposeMacro *m = user_data;
    if (m->ui_row && list_box) {
        gtk_list_box_remove(GTK_LIST_BOX(list_box), GTK_WIDGET(m->ui_row));
    }
    macros = g_list_remove(macros, m);
    free_macro(m);
    save_xcompose();
}

static void add_macro_ui(XComposeMacro *m) {
    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
    g_autofree char *title = g_strdup_printf("%s ➔ %s", m->sequence, m->result);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), m->result);
    adw_action_row_set_subtitle(row, m->sequence);
    m->ui_row = row;

    GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_add_css_class(del_btn, "destructive-action");
    gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
    g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_macro), m);
    adw_action_row_add_suffix(row, del_btn);

    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
}

static void on_add_done(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_seq = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    AdwEntryRow *r_res = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_seq)));
    
    const char *seq = gtk_editable_get_text(GTK_EDITABLE(r_seq));
    const char *res = gtk_editable_get_text(GTK_EDITABLE(r_res));

    if (strlen(seq) > 0 && strlen(res) > 0) {
        XComposeMacro *m = g_new0(XComposeMacro, 1);
        m->sequence = g_strdup(seq);
        m->result = g_strdup(res);
        macros = g_list_append(macros, m);
        add_macro_ui(m);
        save_xcompose();
    }
    adw_dialog_close(dlg);
}

static void on_add_macro(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Add Macro");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);
    
    GtkWidget *lbl = gtk_label_new("Define a new Compose Key combination");
    gtk_box_append(GTK_BOX(box), lbl);
    
    AdwEntryRow *r_seq = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_seq), "Sequence (e.g. <t> <m>)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_seq));
    
    AdwEntryRow *r_res = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_res), "Result (e.g. ™)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_res));
    
    GtkWidget *btn_add = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(btn_add, "suggested-action");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_done), dlg);
    gtk_box_append(GTK_BOX(box), btn_add);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

GtkWidget *page_xcompose_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Text Expansion");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-keyboard-shortcuts-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    load_xcompose();

    AdwPreferencesGroup *hdr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_description(hdr_group, "Configure custom text expansions using the Compose (Multi) key. E.g. <Multi_key> + <t> + <m> = ™");
    adw_preferences_page_add(page, hdr_group);

    AdwActionRow *add_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_row), "Create Expansion");
    GtkWidget *add_btn = gtk_button_new_with_label("Add Custom");
    gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_macro), NULL);
    adw_action_row_add_suffix(add_row, add_btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(add_row));

    list_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(list_group, "Expansions");
    adw_preferences_page_add(page, list_group);

    list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(list_group, list_box);

    for (GList *iter = macros; iter; iter = iter->next) {
        XComposeMacro *m = iter->data;
        if (m->sequence && m->result) add_macro_ui(m);
    }

    return overlay;
}
