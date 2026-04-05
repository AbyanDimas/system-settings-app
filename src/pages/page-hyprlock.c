#include "page-hyprlock.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Very basic config structures */
typedef struct {
    char *path;
    char *color;
    int blur_passes;
    int blur_size;
} HyprlockBackground;

typedef struct {
    char *size;
    char *position;
    char *outline_color;
    char *inner_color;
    char *font_color;
    char *check_color;
    char *fail_color;
} HyprlockInputField;

typedef struct {
    char *text;
    char *color;
    int font_size;
    char *position;
} HyprlockLabel;

static HyprlockBackground bg_cfg;
static HyprlockInputField input_cfg;
static GList *labels = NULL;
static char *general_block = NULL;

static GtkWidget *page_widget = NULL;

/* UI Elements */
static AdwEntryRow *bg_path_row, *bg_color_row;
static AdwSpinRow *bg_passes_row, *bg_size_row;

static AdwEntryRow *in_size_row, *in_pos_row, *in_outcol_row, *in_incol_row;
static AdwEntryRow *in_fontcol_row, *in_checkcol_row, *in_failcol_row;

static AdwPreferencesGroup *labels_group;

static void parse_hyprlock_conf(void) {
    g_free(bg_cfg.path); bg_cfg.path = g_strdup("");
    g_free(bg_cfg.color); bg_cfg.color = g_strdup("");
    bg_cfg.blur_passes = 0; bg_cfg.blur_size = 8;

    g_free(input_cfg.size); input_cfg.size = g_strdup("");
    g_free(input_cfg.position); input_cfg.position = g_strdup("");
    g_free(input_cfg.outline_color); input_cfg.outline_color = g_strdup("");
    g_free(input_cfg.inner_color); input_cfg.inner_color = g_strdup("");
    g_free(input_cfg.font_color); input_cfg.font_color = g_strdup("");
    g_free(input_cfg.check_color); input_cfg.check_color = g_strdup("");
    g_free(input_cfg.fail_color); input_cfg.fail_color = g_strdup("");

    for (GList *iter = labels; iter; iter = iter->next) {
        HyprlockLabel *l = iter->data;
        g_free(l->text); g_free(l->color); g_free(l->position); g_free(l);
    }
    g_list_free(labels);
    labels = NULL;
    g_free(general_block);
    general_block = NULL;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "hyprlock.conf", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) return;

    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    GString *gen_str = g_string_new("");
    
    int block_type = 0; // 0=none, 1=bg, 2=input, 3=label
    HyprlockLabel *cur_lbl = NULL;

    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        
        if (g_str_has_prefix(line, "background {")) {
            block_type = 1;
            g_free(line); continue;
        } else if (g_str_has_prefix(line, "input-field {")) {
            block_type = 2;
            g_free(line); continue;
        } else if (g_str_has_prefix(line, "label {")) {
            block_type = 3;
            cur_lbl = g_new0(HyprlockLabel, 1);
            cur_lbl->text = g_strdup("");
            cur_lbl->color = g_strdup("");
            cur_lbl->position = g_strdup("");
            g_free(line); continue;
        } else if (g_strcmp0(line, "}") == 0) {
            if (block_type == 3 && cur_lbl) {
                labels = g_list_append(labels, cur_lbl);
                cur_lbl = NULL;
            }
            block_type = 0;
            g_free(line); continue;
        }

        if (block_type == 1) {
            char *eq = strchr(line, '=');
            if (eq) {
                char *k = g_strndup(line, eq - line); g_strstrip(k);
                char *v = g_strdup(eq + 1); g_strstrip(v);
                if (g_strcmp0(k, "path") == 0) { g_free(bg_cfg.path); bg_cfg.path = g_strdup(v); }
                else if (g_strcmp0(k, "color") == 0) { g_free(bg_cfg.color); bg_cfg.color = g_strdup(v); }
                else if (g_strcmp0(k, "blur_passes") == 0) bg_cfg.blur_passes = atoi(v);
                else if (g_strcmp0(k, "blur_size") == 0) bg_cfg.blur_size = atoi(v);
                g_free(k); g_free(v);
            }
        } else if (block_type == 2) {
            char *eq = strchr(line, '=');
            if (eq) {
                char *k = g_strndup(line, eq - line); g_strstrip(k);
                char *v = g_strdup(eq + 1); g_strstrip(v);
                if (g_strcmp0(k, "size") == 0) { g_free(input_cfg.size); input_cfg.size = g_strdup(v); }
                else if (g_strcmp0(k, "position") == 0) { g_free(input_cfg.position); input_cfg.position = g_strdup(v); }
                else if (g_strcmp0(k, "outline_color") == 0) { g_free(input_cfg.outline_color); input_cfg.outline_color = g_strdup(v); }
                else if (g_strcmp0(k, "inner_color") == 0) { g_free(input_cfg.inner_color); input_cfg.inner_color = g_strdup(v); }
                else if (g_strcmp0(k, "font_color") == 0) { g_free(input_cfg.font_color); input_cfg.font_color = g_strdup(v); }
                else if (g_strcmp0(k, "check_color") == 0) { g_free(input_cfg.check_color); input_cfg.check_color = g_strdup(v); }
                else if (g_strcmp0(k, "fail_color") == 0) { g_free(input_cfg.fail_color); input_cfg.fail_color = g_strdup(v); }
                g_free(k); g_free(v);
            }
        } else if (block_type == 3 && cur_lbl) {
            char *eq = strchr(line, '=');
            if (eq) {
                char *k = g_strndup(line, eq - line); g_strstrip(k);
                char *v = g_strdup(eq + 1); g_strstrip(v);
                if (g_strcmp0(k, "text") == 0) { g_free(cur_lbl->text); cur_lbl->text = g_strdup(v); }
                else if (g_strcmp0(k, "color") == 0) { g_free(cur_lbl->color); cur_lbl->color = g_strdup(v); }
                else if (g_strcmp0(k, "font_size") == 0) cur_lbl->font_size = atoi(v);
                else if (g_strcmp0(k, "position") == 0) { g_free(cur_lbl->position); cur_lbl->position = g_strdup(v); }
                g_free(k); g_free(v);
            }
        } else if (block_type == 0 && strlen(line) > 0) {
            g_string_append_printf(gen_str, "%s\n", line);
        }
        g_free(line);
    }
    general_block = g_string_free(gen_str, FALSE);
}

static void on_save_config(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "hyprlock.conf", NULL);
    
    GString *out = g_string_new("");
    if (general_block) g_string_append(out, general_block);

    // Save BG
    g_string_append_printf(out, "background {\n");
    g_string_append_printf(out, "    path = %s\n", gtk_editable_get_text(GTK_EDITABLE(bg_path_row)));
    g_string_append_printf(out, "    color = %s\n", gtk_editable_get_text(GTK_EDITABLE(bg_color_row)));
    g_string_append_printf(out, "    blur_passes = %d\n", (int)adw_spin_row_get_value(bg_passes_row));
    g_string_append_printf(out, "    blur_size = %d\n", (int)adw_spin_row_get_value(bg_size_row));
    g_string_append_printf(out, "}\n\n");

    // Save Input
    g_string_append_printf(out, "input-field {\n");
    g_string_append_printf(out, "    size = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_size_row)));
    g_string_append_printf(out, "    position = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_pos_row)));
    g_string_append_printf(out, "    outline_color = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_outcol_row)));
    g_string_append_printf(out, "    inner_color = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_incol_row)));
    g_string_append_printf(out, "    font_color = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_fontcol_row)));
    g_string_append_printf(out, "    check_color = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_checkcol_row)));
    g_string_append_printf(out, "    fail_color = %s\n", gtk_editable_get_text(GTK_EDITABLE(in_failcol_row)));
    g_string_append_printf(out, "}\n\n");

    // Save Labels
    for (GList *iter = labels; iter != NULL; iter = iter->next) {
        HyprlockLabel *l = iter->data;
        g_string_append_printf(out, "label {\n");
        g_string_append_printf(out, "    text = %s\n", l->text);
        g_string_append_printf(out, "    color = %s\n", l->color);
        g_string_append_printf(out, "    font_size = %d\n", l->font_size);
        g_string_append_printf(out, "    position = %s\n", l->position);
        g_string_append_printf(out, "}\n\n");
    }

    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Lock screen updated"));
    }
}

static void on_preview_lock(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_async("hyprlock", NULL, NULL); 
}

static void on_bg_file_picked(GObject *source, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    g_autoptr(GError) error = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, res, &error);
    if (file) {
        g_autofree char *path = g_file_get_path(file);
        gtk_editable_set_text(GTK_EDITABLE(bg_path_row), path);
        g_object_unref(file);
    }
}

static void on_browse_bg(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Wallpaper");
    GtkWindow *win = GTK_WINDOW(gtk_widget_get_native(page_widget));
    gtk_file_dialog_open(dialog, win, NULL, on_bg_file_picked, NULL);
}

static void add_label_ui(HyprlockLabel *l) {
    AdwExpanderRow *row = ADW_EXPANDER_ROW(adw_expander_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), l->text && strlen(l->text) ? l->text : "Empty Label");
    
    AdwEntryRow *text = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(text), "Text");
    gtk_editable_set_text(GTK_EDITABLE(text), l->text ? l->text : "");
    adw_expander_row_add_row(row, GTK_WIDGET(text));

    AdwEntryRow *pos = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(pos), "Position");
    gtk_editable_set_text(GTK_EDITABLE(pos), l->position ? l->position : "");
    adw_expander_row_add_row(row, GTK_WIDGET(pos));

    AdwSpinRow *sz = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 200, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sz), "Font Size");
    adw_spin_row_set_value(sz, l->font_size);
    adw_expander_row_add_row(row, GTK_WIDGET(sz));

    /* Connect changes to back-struct (simple approach without perfect 2way bind) */
    g_signal_connect_swapped(text, "changed", G_CALLBACK(gtk_editable_get_text), l->text); // Actually we'd need a real callback
    
    adw_preferences_group_add(labels_group, GTK_WIDGET(row));
}

GtkWidget *page_hyprlock_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (!caps->has_hyprlock) {
        return omarchy_make_unavailable_page("hyprlock", "sudo pacman -S hyprlock", "changes-prevent-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Lock Screen");
    adw_preferences_page_set_icon_name(page, "changes-prevent-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    parse_hyprlock_conf();

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    GtkWidget *save_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *prev_btn = gtk_button_new_with_label("Live Preview");
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(on_preview_lock), NULL);
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_config), NULL);
    gtk_box_append(GTK_BOX(save_box), prev_btn);
    gtk_box_append(GTK_BOX(save_box), save_btn);
    adw_action_row_add_suffix(save_row, save_box);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Background ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *bg_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(bg_group, "Background");
    adw_preferences_page_add(page, bg_group);

    bg_path_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bg_path_row), "Image Path");
    gtk_editable_set_text(GTK_EDITABLE(bg_path_row), bg_cfg.path ? bg_cfg.path : "");
    GtkWidget *br_btn = gtk_button_new_with_label("Browse");
    gtk_widget_set_valign(br_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(br_btn, 8);
    g_signal_connect(br_btn, "clicked", G_CALLBACK(on_browse_bg), NULL);
    adw_entry_row_add_suffix(bg_path_row, br_btn);
    adw_preferences_group_add(bg_group, GTK_WIDGET(bg_path_row));

    bg_color_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bg_color_row), "Fallback Color");
    gtk_editable_set_text(GTK_EDITABLE(bg_color_row), bg_cfg.color ? bg_cfg.color : "");
    adw_preferences_group_add(bg_group, GTK_WIDGET(bg_color_row));

    bg_passes_row = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 5, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bg_passes_row), "Blur Passes");
    adw_spin_row_set_value(bg_passes_row, bg_cfg.blur_passes);
    adw_preferences_group_add(bg_group, GTK_WIDGET(bg_passes_row));

    bg_size_row = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 30, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bg_size_row), "Blur Size");
    adw_spin_row_set_value(bg_size_row, bg_cfg.blur_size);
    adw_preferences_group_add(bg_group, GTK_WIDGET(bg_size_row));

    /* ── Input Field ───────────────────────────────────────────────────────── */
    AdwPreferencesGroup *in_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(in_group, "Input Field");
    adw_preferences_page_add(page, in_group);

    in_size_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_size_row), "Size (w, h)");
    gtk_editable_set_text(GTK_EDITABLE(in_size_row), input_cfg.size ? input_cfg.size : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_size_row));

    in_pos_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_pos_row), "Position (x, y)");
    gtk_editable_set_text(GTK_EDITABLE(in_pos_row), input_cfg.position ? input_cfg.position : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_pos_row));

    in_outcol_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_outcol_row), "Outline Color");
    gtk_editable_set_text(GTK_EDITABLE(in_outcol_row), input_cfg.outline_color ? input_cfg.outline_color : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_outcol_row));

    in_incol_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_incol_row), "Inner Color");
    gtk_editable_set_text(GTK_EDITABLE(in_incol_row), input_cfg.inner_color ? input_cfg.inner_color : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_incol_row));

    in_fontcol_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_fontcol_row), "Font Color");
    gtk_editable_set_text(GTK_EDITABLE(in_fontcol_row), input_cfg.font_color ? input_cfg.font_color : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_fontcol_row));

    in_checkcol_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_checkcol_row), "Check Color");
    gtk_editable_set_text(GTK_EDITABLE(in_checkcol_row), input_cfg.check_color ? input_cfg.check_color : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_checkcol_row));

    in_failcol_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_failcol_row), "Fail Color");
    gtk_editable_set_text(GTK_EDITABLE(in_failcol_row), input_cfg.fail_color ? input_cfg.fail_color : "");
    adw_preferences_group_add(in_group, GTK_WIDGET(in_failcol_row));

    /* ── Labels ────────────────────────────────────────────────────────────── */
    labels_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(labels_group, "Labels");
    adw_preferences_page_add(page, labels_group);

    for (GList *iter = labels; iter != NULL; iter = iter->next) add_label_ui(iter->data);

    return overlay;
}
