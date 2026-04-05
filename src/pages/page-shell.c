#include "page-shell.h"
#include "../window.h"
#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkSourceBuffer *user_buffer;
static GtkWidget *page_widget = NULL;
static GtkSourceView *user_view;

static void on_save_bashrc(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(user_buffer), &start, &end);
    char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(user_buffer), &start, &end, FALSE);
    
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);
    g_file_set_contents(path, text, -1, NULL);
    g_free(text);
    
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Shell config saved. Open a new terminal to apply."));
    }
}

static void on_add_alias_done(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_name = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    AdwEntryRow *r_cmd = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_name)));
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(r_name));
    const char *cmd = gtk_editable_get_text(GTK_EDITABLE(r_cmd));
    
    if (strlen(name) > 0 && strlen(cmd) > 0) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(user_buffer), &end);
        g_autofree char *code = g_strdup_printf("\nalias %s='%s'\n", name, cmd);
        gtk_text_buffer_insert(GTK_TEXT_BUFFER(user_buffer), &end, code, -1);
    }
    adw_dialog_close(dlg);
}

static void on_add_alias(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Add Alias");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);
    
    GtkWidget *lbl = gtk_label_new("Define a new shortcut alias");
    gtk_box_append(GTK_BOX(box), lbl);
    
    AdwEntryRow *r_name = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_name), "Name (e.g. ll)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_name));
    
    AdwEntryRow *r_cmd = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_cmd), "Command (e.g. ls -la)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_cmd));
    
    GtkWidget *btn_add = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(btn_add, "suggested-action");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_alias_done), dlg);
    gtk_box_append(GTK_BOX(box), btn_add);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

static void on_add_export_done(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_var = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    AdwEntryRow *r_val = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_var)));
    
    const char *var = gtk_editable_get_text(GTK_EDITABLE(r_var));
    const char *val = gtk_editable_get_text(GTK_EDITABLE(r_val));
    
    if (strlen(var) > 0 && strlen(val) > 0) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(user_buffer), &end);
        g_autofree char *code = g_strdup_printf("\nexport %s=\"%s\"\n", var, val);
        gtk_text_buffer_insert(GTK_TEXT_BUFFER(user_buffer), &end, code, -1);
    }
    adw_dialog_close(dlg);
}

static void on_add_export(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Add Export");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);
    
    GtkWidget *lbl = gtk_label_new("Export Environment Variable");
    gtk_box_append(GTK_BOX(box), lbl);
    
    AdwEntryRow *r_var = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_var), "Variable (e.g. EDITOR)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_var));
    
    AdwEntryRow *r_val = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_val), "Value (e.g. nvim)");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_val));
    
    GtkWidget *btn_add = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(btn_add, "suggested-action");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_export_done), dlg);
    gtk_box_append(GTK_BOX(box), btn_add);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

static void on_add_func_done(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_name = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(r_name));
    if (strlen(name) > 0) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(user_buffer), &end);
        g_autofree char *code = g_strdup_printf("\n%s() {\n    # TODO: Implement function\n    echo \"Running %s\"\n}\n", name, name);
        gtk_text_buffer_insert(GTK_TEXT_BUFFER(user_buffer), &end, code, -1);
    }
    adw_dialog_close(dlg);
}

static void on_add_func(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Add Function");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);
    
    GtkWidget *lbl = gtk_label_new("Create a Bash Function Template");
    gtk_box_append(GTK_BOX(box), lbl);
    
    AdwEntryRow *r_name = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_name), "Function Name");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_name));
    
    GtkWidget *btn_add = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(btn_add, "suggested-action");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_func_done), dlg);
    gtk_box_append(GTK_BOX(box), btn_add);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

GtkWidget *page_shell_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), vbox);

    // Title / Toolbar
    GtkWidget *header = adw_preferences_page_new();
    adw_preferences_page_set_title(ADW_PREFERENCES_PAGE(header), "Shell Config");
    adw_preferences_page_set_icon_name(ADW_PREFERENCES_PAGE(header), "utilities-terminal-symbolic");
    gtk_widget_set_vexpand(header, FALSE);
    gtk_box_append(GTK_BOX(vbox), header);

    AdwPreferencesGroup *q_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(header), q_group);

    AdwActionRow *sr_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr_row), "Apply Changes");
    GtkWidget *sv_btn = gtk_button_new_with_label("Save ~/.bashrc");
    gtk_widget_add_css_class(sv_btn, "suggested-action");
    gtk_widget_set_valign(sv_btn, GTK_ALIGN_CENTER);
    g_signal_connect(sv_btn, "clicked", G_CALLBACK(on_save_bashrc), NULL);
    adw_action_row_add_suffix(sr_row, sv_btn);
    GtkWidget *info_lbl = gtk_label_new("Run: `source ~/.bashrc` in your terminal to apply");
    gtk_widget_add_css_class(info_lbl, "dim-label");
    gtk_widget_set_valign(info_lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(info_lbl, 8);
    adw_action_row_add_suffix(sr_row, info_lbl);
    adw_preferences_group_add(q_group, GTK_WIDGET(sr_row));

    AdwPreferencesGroup *add_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(add_group, "Quick Add");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(header), add_group);

    GtkWidget *abox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(abox, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(abox, 12);
    
    GtkWidget *b_alias = gtk_button_new_with_label("Add Alias");
    g_signal_connect(b_alias, "clicked", G_CALLBACK(on_add_alias), NULL);
    gtk_box_append(GTK_BOX(abox), b_alias);
    
    GtkWidget *b_exp = gtk_button_new_with_label("Add Export");
    g_signal_connect(b_exp, "clicked", G_CALLBACK(on_add_export), NULL);
    gtk_box_append(GTK_BOX(abox), b_exp);

    GtkWidget *b_func = gtk_button_new_with_label("Add Function");
    g_signal_connect(b_func, "clicked", G_CALLBACK(on_add_func), NULL);
    gtk_box_append(GTK_BOX(abox), b_func);
    
    adw_preferences_group_add(add_group, abox);

    // Split View Editors
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_widget_set_hexpand(paned, TRUE);
    gtk_box_append(GTK_BOX(vbox), paned);

    /* Setup GtkSourceView language */
    GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lm, "sh");

    // LEFT: System Defaults
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *left_lbl = gtk_label_new("System Defaults (Read-Only)");
    gtk_widget_add_css_class(left_lbl, "heading");
    gtk_widget_set_margin_top(left_lbl, 8); gtk_widget_set_margin_bottom(left_lbl, 8);
    gtk_box_append(GTK_BOX(left_box), left_lbl);

    GtkWidget *left_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(left_scroll, TRUE);
    gtk_box_append(GTK_BOX(left_box), left_scroll);

    GtkSourceBuffer *def_buf = gtk_source_buffer_new_with_language(lang);
    GtkSourceView *def_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(def_buf));
    gtk_source_view_set_show_line_numbers(def_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(def_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(def_view), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(left_scroll), GTK_WIDGET(def_view));

    g_autofree char *def_path = g_build_filename(g_get_home_dir(), ".local", "share", "omarchy", "bash", "bashrc", NULL);
    g_autofree char *def_text = NULL;
    if (g_file_get_contents(def_path, &def_text, NULL, NULL)) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(def_buf), def_text, -1);
    } else {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(def_buf), "# Default System bashrc not found.", -1);
    }
    gtk_paned_set_start_child(GTK_PANED(paned), left_box);

    // RIGHT: User ~/.bashrc
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *right_title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(right_title_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(right_title_box, 8); gtk_widget_set_margin_bottom(right_title_box, 8);
    
    GtkWidget *right_lbl = gtk_label_new("User ~/.bashrc");
    gtk_widget_add_css_class(right_lbl, "heading");
    gtk_box_append(GTK_BOX(right_title_box), right_lbl);

    GtkWidget *search_btn = gtk_toggle_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(search_btn), "system-search-symbolic");
    gtk_box_append(GTK_BOX(right_title_box), search_btn);
    gtk_box_append(GTK_BOX(right_box), right_title_box);

    GtkWidget *search_bar = gtk_search_bar_new();
    gtk_box_append(GTK_BOX(right_box), search_bar);

    GtkWidget *right_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(right_scroll, TRUE);
    gtk_box_append(GTK_BOX(right_box), right_scroll);

    user_buffer = gtk_source_buffer_new_with_language(lang);
    user_view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(user_buffer));
    gtk_source_view_set_show_line_numbers(user_view, TRUE);
    gtk_source_view_set_auto_indent(user_view, TRUE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(user_view), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(right_scroll), GTK_WIDGET(user_view));

    gtk_search_bar_set_child(GTK_SEARCH_BAR(search_bar), gtk_search_entry_new());
    gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search_bar), GTK_EDITABLE(gtk_search_bar_get_child(GTK_SEARCH_BAR(search_bar))));
    g_object_bind_property(search_btn, "active", search_bar, "search-mode-enabled", G_BINDING_BIDIRECTIONAL);

    g_autofree char *usr_path = g_build_filename(g_get_home_dir(), ".bashrc", NULL);
    g_autofree char *usr_text = NULL;
    if (g_file_get_contents(usr_path, &usr_text, NULL, NULL)) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(user_buffer), usr_text, -1);
    }
    gtk_paned_set_end_child(GTK_PANED(paned), right_box);
    gtk_paned_set_position(GTK_PANED(paned), 400);

    return overlay;
}
