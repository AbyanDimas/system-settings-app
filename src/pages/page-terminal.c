#include "page-terminal.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOML_LINE_SECTION 1
#define TOML_LINE_KEYVAL  2
#define TOML_LINE_OTHER   0

typedef struct {
    int type;
    char *section;
    char *key;
    char *value;
    char *raw_text;
} TomlLine;

static GList *toml_lines = NULL;

static GtkWidget *page_widget = NULL;

/* Extracted config state */
static char *font_family = NULL;
static double font_size = 11.0;
static int offset_x = 0;
static int offset_y = 0;

static double opacity = 1.0;
static int padding_x = 0;
static int padding_y = 0;
static char *decorations = NULL;
static char *startup_mode = NULL;

static int history = 10000;
static char *cursor_style = NULL;
static char *cursor_blink = NULL;

static char *bell_anim = NULL;
static int bell_dur = 0;

static void free_toml_line(TomlLine *l) {
    g_free(l->section);
    g_free(l->key);
    g_free(l->value);
    g_free(l->raw_text);
    g_free(l);
}

static void load_toml(void) {
    g_list_free_full(toml_lines, (GDestroyNotify)free_toml_line);
    toml_lines = NULL;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "alacritty", "alacritty.toml", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) return;

    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    char *current_section = g_strdup("");

    for (int i = 0; lines[i] != NULL; i++) {
        TomlLine *l = g_new0(TomlLine, 1);
        l->raw_text = g_strdup(lines[i]);
        char *t = g_strstrip(g_strdup(lines[i]));

        if (g_str_has_prefix(t, "[")) {
            char *end = strchr(t, ']');
            if (end) {
                l->type = TOML_LINE_SECTION;
                g_free(current_section);
                current_section = g_strndup(t + 1, end - t - 1);
                l->section = g_strdup(current_section);
            } else {
                l->type = TOML_LINE_OTHER;
            }
        } else {
            char *eq = strchr(t, '=');
            if (eq && t[0] != '#') {
                l->type = TOML_LINE_KEYVAL;
                l->section = g_strdup(current_section);
                l->key = g_strndup(t, eq - t); g_strstrip(l->key);
                l->value = g_strdup(eq + 1); g_strstrip(l->value);
            } else {
                l->type = TOML_LINE_OTHER;
            }
        }
        toml_lines = g_list_append(toml_lines, l);
        g_free(t);
    }
    g_free(current_section);

    // Initial defaults
    font_family = g_strdup("monospace");
    decorations = g_strdup("full");
    startup_mode = g_strdup("Windowed");
    cursor_style = g_strdup("Block");
    cursor_blink = g_strdup("Off");
    bell_anim = g_strdup("EaseOutExpo");

    // Map extracted config
    for (GList *iter = toml_lines; iter; iter = iter->next) {
        TomlLine *l = iter->data;
        if (l->type != TOML_LINE_KEYVAL) continue;
        
        // Strip quotes
        gboolean is_str = (l->value[0] == '"' || l->value[0] == '\'');
        char *v = l->value;
        if (is_str) {
            v = g_strdup(l->value + 1);
            if (strlen(v) > 0) v[strlen(v)-1] = '\0';
        } else {
            v = g_strdup(l->value);
        }

        if (g_strcmp0(l->section, "font.normal") == 0 && g_strcmp0(l->key, "family") == 0) {
            g_free(font_family); font_family = g_strdup(v);
        } else if (g_strcmp0(l->section, "font") == 0) {
            if (g_strcmp0(l->key, "size") == 0) font_size = atof(v);
        } else if (g_strcmp0(l->section, "font.offset") == 0) {
            if (g_strcmp0(l->key, "x") == 0) offset_x = atoi(v);
            else if (g_strcmp0(l->key, "y") == 0) offset_y = atoi(v);
        } else if (g_strcmp0(l->section, "window") == 0) {
            if (g_strcmp0(l->key, "opacity") == 0) opacity = atof(v);
            else if (g_strcmp0(l->key, "decorations") == 0) { g_free(decorations); decorations = g_strdup(v); }
            else if (g_strcmp0(l->key, "startup_mode") == 0) { g_free(startup_mode); startup_mode = g_strdup(v); }
        } else if (g_strcmp0(l->section, "window.padding") == 0) {
            if (g_strcmp0(l->key, "x") == 0) padding_x = atoi(v);
            else if (g_strcmp0(l->key, "y") == 0) padding_y = atoi(v);
        } else if (g_strcmp0(l->section, "scrolling") == 0 && g_strcmp0(l->key, "history") == 0) {
            history = atoi(v);
        } else if (g_strcmp0(l->section, "cursor.style") == 0) {
            if (g_strcmp0(l->key, "shape") == 0) { g_free(cursor_style); cursor_style = g_strdup(v); }
            else if (g_strcmp0(l->key, "blinking") == 0) { g_free(cursor_blink); cursor_blink = g_strdup(v); }
        } else if (g_strcmp0(l->section, "bell") == 0) {
            if (g_strcmp0(l->key, "animation") == 0) { g_free(bell_anim); bell_anim = g_strdup(v); }
            else if (g_strcmp0(l->key, "duration") == 0) bell_dur = atoi(v);
        }

        if (is_str) g_free(v);
    }
}

static void update_or_add_toml(const char *section, const char *key, const char *val, gboolean is_string) {
    g_autofree char *formatted_val = is_string ? g_strdup_printf("\"%s\"", val) : g_strdup(val);
    gboolean section_found = FALSE;
    GList *last_in_section = NULL;

    for (GList *iter = toml_lines; iter; iter = iter->next) {
        TomlLine *l = iter->data;
        if (l->section && g_strcmp0(l->section, section) == 0) {
            section_found = TRUE;
            last_in_section = iter;
            if (l->type == TOML_LINE_KEYVAL && g_strcmp0(l->key, key) == 0) {
                g_free(l->value);
                l->value = g_strdup(formatted_val);
                g_free(l->raw_text);
                l->raw_text = g_strdup_printf("%s = %s", key, formatted_val);
                return;
            }
        }
    }

    // Add new
    TomlLine *nl = g_new0(TomlLine, 1);
    nl->type = TOML_LINE_KEYVAL;
    nl->section = g_strdup(section);
    nl->key = g_strdup(key);
    nl->value = g_strdup(formatted_val);
    nl->raw_text = g_strdup_printf("%s = %s", key, formatted_val);

    if (section_found && last_in_section) {
        toml_lines = g_list_insert_before(toml_lines, last_in_section->next, nl);
    } else {
        // Create section at EOF
        TomlLine *sl = g_new0(TomlLine, 1);
        sl->type = TOML_LINE_SECTION;
        sl->section = g_strdup(section);
        sl->raw_text = g_strdup_printf("[%s]", section);
        toml_lines = g_list_append(toml_lines, sl);
        toml_lines = g_list_append(toml_lines, nl);
    }
}

static AdwEntryRow *ui_font_fam;
static AdwSpinRow *ui_font_sz, *ui_off_x, *ui_off_y;
static GtkScale *ui_opacity;
static AdwSpinRow *ui_pad_x, *ui_pad_y, *ui_hist, *ui_bell_dur;
static AdwComboRow *ui_dec, *ui_start, *ui_cur_shape, *ui_cur_blink, *ui_bell_anim;

static void on_save_terminal(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;

    update_or_add_toml("font.normal", "family", gtk_editable_get_text(GTK_EDITABLE(ui_font_fam)), TRUE);
    g_autofree char *f_sz = g_strdup_printf("%.1f", adw_spin_row_get_value(ui_font_sz));
    update_or_add_toml("font", "size", f_sz, FALSE);
    g_autofree char *o_x = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_off_x));
    g_autofree char *o_y = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_off_y));
    update_or_add_toml("font.offset", "x", o_x, FALSE);
    update_or_add_toml("font.offset", "y", o_y, FALSE);

    g_autofree char *op = g_strdup_printf("%.2f", gtk_range_get_value(GTK_RANGE(ui_opacity)));
    update_or_add_toml("window", "opacity", op, FALSE);

    g_autofree char *p_x = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_pad_x));
    g_autofree char *p_y = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_pad_y));
    update_or_add_toml("window.padding", "x", p_x, FALSE);
    update_or_add_toml("window.padding", "y", p_y, FALSE);

    const char *decs[] = {"full", "none", "transparent", "buttonless"};
    update_or_add_toml("window", "decorations", decs[adw_combo_row_get_selected(ui_dec)], TRUE);
    const char *stm[] = {"Windowed", "Maximized", "Fullscreen", "SimpleFullscreen"};
    update_or_add_toml("window", "startup_mode", stm[adw_combo_row_get_selected(ui_start)], TRUE);

    g_autofree char *hist = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_hist));
    update_or_add_toml("scrolling", "history", hist, FALSE);

    const char *csh[] = {"Block", "Underline", "Beam"};
    update_or_add_toml("cursor.style", "shape", csh[adw_combo_row_get_selected(ui_cur_shape)], TRUE);
    const char *cbl[] = {"Never", "Off", "On", "Always"};
    update_or_add_toml("cursor.style", "blinking", cbl[adw_combo_row_get_selected(ui_cur_blink)], TRUE);

    const char *banm[] = {"Ease", "EaseOut", "EaseOutSine", "EaseOutQuad", "EaseOutCubic", "EaseOutQuart", "EaseOutQuint", "EaseOutExpo", "EaseOutCirc"};
    update_or_add_toml("bell", "animation", banm[adw_combo_row_get_selected(ui_bell_anim)], TRUE);
    g_autofree char *bdur = g_strdup_printf("%d", (int)adw_spin_row_get_value(ui_bell_dur));
    update_or_add_toml("bell", "duration", bdur, FALSE);

    // Save
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "alacritty", "alacritty.toml", NULL);
    GString *out = g_string_new("");
    for (GList *iter = toml_lines; iter; iter = iter->next) {
        TomlLine *l = iter->data;
        g_string_append_printf(out, "%s\n", l->raw_text);
    }
    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Terminal config saved. Restart Alacritty to apply."));
    }
}

static void on_font_picked(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    if (!row) return;
    GtkWidget *label = gtk_list_box_row_get_child(row);
    gtk_editable_set_text(GTK_EDITABLE(ui_font_fam), gtk_label_get_text(GTK_LABEL(label)));
    AdwDialog *dlg = ADW_DIALOG(user_data);
    adw_dialog_close(dlg);
}

static void on_browse_fonts(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_content_width(dlg, 400);
    adw_dialog_set_content_height(dlg, 500);
    adw_dialog_set_title(dlg, "Select Font");

    GtkWidget *scroll = gtk_scrolled_window_new();
    GtkWidget *list = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);

    g_autoptr(GError) err = NULL;
    g_autofree char *f_out = subprocess_run_sync("fc-list :family | cut -d',' -f1 | sort -u", &err);
    if (f_out) {
        g_auto(GStrv) fonts = g_strsplit(f_out, "\n", -1);
        for (int i = 0; fonts[i] != NULL; i++) {
            if (strlen(fonts[i]) > 0) {
                GtkWidget *lbl = gtk_label_new(fonts[i]);
                gtk_widget_set_halign(lbl, GTK_ALIGN_START);
                gtk_widget_set_margin_start(lbl, 16);
                gtk_widget_set_margin_end(lbl, 16);
                gtk_widget_set_margin_top(lbl, 8);
                gtk_widget_set_margin_bottom(lbl, 8);
                gtk_list_box_append(GTK_LIST_BOX(list), lbl);
            }
        }
    }

    g_signal_connect(list, "row-activated", G_CALLBACK(on_font_picked), dlg);
    adw_dialog_set_child(dlg, scroll);
    adw_dialog_present(dlg, page_widget);
}

GtkWidget *page_terminal_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->terminal, "none") == 0) {
        return omarchy_make_unavailable_page("Terminal Emulator", "sudo pacman -S alacritty", "utilities-terminal-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Terminal");
    adw_preferences_page_set_icon_name(page, "utilities-terminal-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    load_toml();

    /* ── Top controls ──────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    GtkWidget *save_btn = gtk_button_new_with_label("Save Config");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_terminal), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Font ─────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *f_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(f_group, "Font");
    adw_preferences_page_add(page, f_group);

    ui_font_fam = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_font_fam), "Family");
    gtk_editable_set_text(GTK_EDITABLE(ui_font_fam), font_family);
    GtkWidget *br_f_btn = gtk_button_new_with_label("Browse");
    gtk_widget_set_valign(br_f_btn, GTK_ALIGN_CENTER);
    g_signal_connect(br_f_btn, "clicked", G_CALLBACK(on_browse_fonts), NULL);
    adw_entry_row_add_suffix(ui_font_fam, br_f_btn);
    adw_preferences_group_add(f_group, GTK_WIDGET(ui_font_fam));

    ui_font_sz = ADW_SPIN_ROW(adw_spin_row_new_with_range(6.0, 32.0, 0.5));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_font_sz), "Size");
    adw_spin_row_set_value(ui_font_sz, font_size);
    adw_preferences_group_add(f_group, GTK_WIDGET(ui_font_sz));

    ui_off_x = ADW_SPIN_ROW(adw_spin_row_new_with_range(-50, 50, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_off_x), "Offset X");
    adw_spin_row_set_value(ui_off_x, offset_x);
    adw_preferences_group_add(f_group, GTK_WIDGET(ui_off_x));

    ui_off_y = ADW_SPIN_ROW(adw_spin_row_new_with_range(-50, 50, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_off_y), "Offset Y");
    adw_spin_row_set_value(ui_off_y, offset_y);
    adw_preferences_group_add(f_group, GTK_WIDGET(ui_off_y));

    /* ── Window ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *w_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(w_group, "Window");
    adw_preferences_page_add(page, w_group);

    AdwActionRow *op_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(op_row), "Opacity");
    ui_opacity = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));
    gtk_range_set_value(GTK_RANGE(ui_opacity), opacity);
    gtk_scale_set_draw_value(ui_opacity, TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(ui_opacity), 200, -1);
    gtk_widget_set_valign(GTK_WIDGET(ui_opacity), GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(op_row, GTK_WIDGET(ui_opacity));
    adw_preferences_group_add(w_group, GTK_WIDGET(op_row));

    ui_pad_x = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 100, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_pad_x), "Padding X");
    adw_spin_row_set_value(ui_pad_x, padding_x);
    adw_preferences_group_add(w_group, GTK_WIDGET(ui_pad_x));

    ui_pad_y = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 100, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_pad_y), "Padding Y");
    adw_spin_row_set_value(ui_pad_y, padding_y);
    adw_preferences_group_add(w_group, GTK_WIDGET(ui_pad_y));

    ui_dec = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_dec), "Decorations");
    const char *decs[] = {"full", "none", "transparent", "buttonless"};
    GtkStringList *d_mod = gtk_string_list_new(NULL);
    int d_idx = 0;
    for(int i=0; i<4; i++) { gtk_string_list_append(d_mod, decs[i]); if(g_strcmp0(decorations, decs[i])==0) d_idx=i; }
    adw_combo_row_set_model(ui_dec, G_LIST_MODEL(d_mod));
    adw_combo_row_set_selected(ui_dec, d_idx);
    adw_preferences_group_add(w_group, GTK_WIDGET(ui_dec));

    ui_start = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_start), "Startup Mode");
    const char *stms[] = {"Windowed", "Maximized", "Fullscreen", "SimpleFullscreen"};
    GtkStringList *s_mod = gtk_string_list_new(NULL);
    int s_idx = 0;
    for(int i=0; i<4; i++) { gtk_string_list_append(s_mod, stms[i]); if(g_strcmp0(startup_mode, stms[i])==0) s_idx=i; }
    adw_combo_row_set_model(ui_start, G_LIST_MODEL(s_mod));
    adw_combo_row_set_selected(ui_start, s_idx);
    adw_preferences_group_add(w_group, GTK_WIDGET(ui_start));

    /* ── Misc ─────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *m_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(m_group, "Scrolling & Cursor & Bell");
    adw_preferences_page_add(page, m_group);

    ui_hist = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 100000, 1000));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_hist), "History (Scrollback)");
    adw_spin_row_set_value(ui_hist, history);
    adw_preferences_group_add(m_group, GTK_WIDGET(ui_hist));

    ui_cur_shape = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_cur_shape), "Cursor Style");
    const char *cshs[] = {"Block", "Underline", "Beam"};
    GtkStringList *cs_mod = gtk_string_list_new(NULL);
    int cs_idx = 0;
    for(int i=0; i<3; i++) { gtk_string_list_append(cs_mod, cshs[i]); if(g_strcmp0(cursor_style, cshs[i])==0) cs_idx=i; }
    adw_combo_row_set_model(ui_cur_shape, G_LIST_MODEL(cs_mod));
    adw_combo_row_set_selected(ui_cur_shape, cs_idx);
    adw_preferences_group_add(m_group, GTK_WIDGET(ui_cur_shape));

    ui_cur_blink = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_cur_blink), "Cursor Blinking");
    const char *cbls[] = {"Never", "Off", "On", "Always"};
    GtkStringList *cb_mod = gtk_string_list_new(NULL);
    int cb_idx = 0;
    for(int i=0; i<4; i++) { gtk_string_list_append(cb_mod, cbls[i]); if(g_strcmp0(cursor_blink, cbls[i])==0) cb_idx=i; }
    adw_combo_row_set_model(ui_cur_blink, G_LIST_MODEL(cb_mod));
    adw_combo_row_set_selected(ui_cur_blink, cb_idx);
    adw_preferences_group_add(m_group, GTK_WIDGET(ui_cur_blink));

    ui_bell_anim = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_bell_anim), "Bell Animation");
    const char *banms[] = {"Ease", "EaseOut", "EaseOutSine", "EaseOutQuad", "EaseOutCubic", "EaseOutQuart", "EaseOutQuint", "EaseOutExpo", "EaseOutCirc"};
    GtkStringList *ba_mod = gtk_string_list_new(NULL);
    int b_idx = 0;
    for(int i=0; i<9; i++) { gtk_string_list_append(ba_mod, banms[i]); if(g_strcmp0(bell_anim, banms[i])==0) b_idx=i; }
    adw_combo_row_set_model(ui_bell_anim, G_LIST_MODEL(ba_mod));
    adw_combo_row_set_selected(ui_bell_anim, b_idx);
    adw_preferences_group_add(m_group, GTK_WIDGET(ui_bell_anim));

    ui_bell_dur = ADW_SPIN_ROW(adw_spin_row_new_with_range(0, 1000, 10));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_bell_dur), "Bell Duration (ms)");
    adw_spin_row_set_value(ui_bell_dur, bell_dur);
    adw_preferences_group_add(m_group, GTK_WIDGET(ui_bell_dur));

    return overlay;
}
