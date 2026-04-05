#include "page-walker.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

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

static char *w_placeholder = NULL;
static gboolean w_typeahead = FALSE;
static gboolean w_show_initial = TRUE;
static char *w_terminal = NULL;
static char *w_theme = NULL;

static gboolean p_apps = TRUE;
static gboolean p_runner = TRUE;
static gboolean p_commands = TRUE;
static gboolean p_ssh = TRUE;

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

    w_placeholder = g_strdup("Search...");
    w_typeahead = FALSE;
    w_show_initial = TRUE;
    w_terminal = g_strdup("alacritty");
    w_theme = g_strdup("default");
    
    p_apps = TRUE; p_runner = TRUE; p_commands = TRUE; p_ssh = TRUE;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "walker", "config.toml", NULL);
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
            } else l->type = TOML_LINE_OTHER;
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

    for (GList *iter = toml_lines; iter; iter = iter->next) {
        TomlLine *l = iter->data;
        if (l->type != TOML_LINE_KEYVAL) continue;
        
        gboolean is_str = (l->value[0] == '"' || l->value[0] == '\'');
        char *v = l->value;
        if (is_str) {
            v = g_strdup(l->value + 1);
            if (strlen(v) > 0) v[strlen(v)-1] = '\0';
        } else v = g_strdup(l->value);

        gboolean b_val = (g_strcmp0(v, "true") == 0);

        if (g_strcmp0(l->section, "") == 0 && g_strcmp0(l->key, "theme") == 0) {
            g_free(w_theme); w_theme = g_strdup(v);
        } else if (g_strcmp0(l->section, "search") == 0) {
            if (g_strcmp0(l->key, "placeholder") == 0) { g_free(w_placeholder); w_placeholder = g_strdup(v); }
            else if (g_strcmp0(l->key, "typeahead") == 0) w_typeahead = b_val;
            else if (g_strcmp0(l->key, "show_initial_entries") == 0) w_show_initial = b_val;
        } else if (g_strcmp0(l->section, "terminal") == 0 && g_strcmp0(l->key, "emulator") == 0) {
            g_free(w_terminal); w_terminal = g_strdup(v);
        } else if (g_strcmp0(l->section, "plugins.applications") == 0 && g_strcmp0(l->key, "enabled") == 0) {
            p_apps = b_val;
        } else if (g_strcmp0(l->section, "plugins.runner") == 0 && g_strcmp0(l->key, "enabled") == 0) {
            p_runner = b_val;
        } else if (g_strcmp0(l->section, "plugins.commands") == 0 && g_strcmp0(l->key, "enabled") == 0) {
            p_commands = b_val;
        } else if (g_strcmp0(l->section, "plugins.ssh") == 0 && g_strcmp0(l->key, "enabled") == 0) {
            p_ssh = b_val;
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
                g_free(l->value); l->value = g_strdup(formatted_val);
                g_free(l->raw_text); l->raw_text = g_strdup_printf("%s = %s", key, formatted_val);
                return;
            }
        }
    }

    TomlLine *nl = g_new0(TomlLine, 1);
    nl->type = TOML_LINE_KEYVAL;
    nl->section = g_strdup(section);
    nl->key = g_strdup(key);
    nl->value = g_strdup(formatted_val);
    nl->raw_text = g_strdup_printf("%s = %s", key, formatted_val);

    if (section_found && last_in_section) {
        toml_lines = g_list_insert_before(toml_lines, last_in_section->next, nl);
    } else {
        if (strlen(section) > 0) {
            TomlLine *sl = g_new0(TomlLine, 1);
            sl->type = TOML_LINE_SECTION;
            sl->section = g_strdup(section);
            sl->raw_text = g_strdup_printf("[%s]", section);
            toml_lines = g_list_append(toml_lines, sl);
        }
        toml_lines = g_list_append(toml_lines, nl);
    }
}

static AdwEntryRow *ui_srch_ph, *ui_term;
static AdwSwitchRow *ui_srch_t_head, *ui_srch_init;
static AdwComboRow *ui_theme;
static AdwSwitchRow *s_apps, *s_run, *s_cmd, *s_ssh;

static GtkStringList *theme_mod = NULL;

static void on_save_walker(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;

    update_or_add_toml("search", "placeholder", gtk_editable_get_text(GTK_EDITABLE(ui_srch_ph)), TRUE);
    update_or_add_toml("search", "typeahead", adw_switch_row_get_active(ui_srch_t_head) ? "true":"false", FALSE);
    update_or_add_toml("search", "show_initial_entries", adw_switch_row_get_active(ui_srch_init) ? "true":"false", FALSE);

    update_or_add_toml("terminal", "emulator", gtk_editable_get_text(GTK_EDITABLE(ui_term)), TRUE);

    const char *sel_theme = gtk_string_list_get_string(theme_mod, adw_combo_row_get_selected(ui_theme));
    if (sel_theme) update_or_add_toml("", "theme", sel_theme, TRUE);

    update_or_add_toml("plugins.applications", "enabled", adw_switch_row_get_active(s_apps) ? "true":"false", FALSE);
    update_or_add_toml("plugins.runner", "enabled", adw_switch_row_get_active(s_run) ? "true":"false", FALSE);
    update_or_add_toml("plugins.commands", "enabled", adw_switch_row_get_active(s_cmd) ? "true":"false", FALSE);
    update_or_add_toml("plugins.ssh", "enabled", adw_switch_row_get_active(s_ssh) ? "true":"false", FALSE);

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "walker", "config.toml", NULL);
    GString *out = g_string_new("");
    for (GList *iter = toml_lines; iter; iter = iter->next) {
        TomlLine *l = iter->data;
        g_string_append_printf(out, "%s\n", l->raw_text);
    }
    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Walker config saved. Restart Walker to apply."));
    }
}

GtkWidget *page_walker_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->app_launcher, "walker") != 0 && !caps->has_walker) {
        return omarchy_make_unavailable_page("Walker", "sudo pacman -S walker", "system-search-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "App Launcher");
    adw_preferences_page_set_icon_name(page, "system-search-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    load_toml();

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    GtkWidget *save_btn = gtk_button_new_with_label("Save Config");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_walker), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Search & Theme ────────────────────────────────────────────────────── */
    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "General");
    adw_preferences_page_add(page, st_group);

    ui_theme = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_theme), "Theme");
    theme_mod = gtk_string_list_new(NULL);
    
    // Read themes from ~/.local/share/walker/themes/ or fallback
    gtk_string_list_append(theme_mod, "default");
    gtk_string_list_append(theme_mod, "omarchy");
    
    g_autofree char *t_dir = g_build_filename(g_get_home_dir(), ".local", "share", "walker", "themes", NULL);
    g_autoptr(GFile) d = g_file_new_for_path(t_dir);
    g_autoptr(GFileEnumerator) e = g_file_enumerate_children(d, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, NULL);
    if (e) {
        GFileInfo *info;
        while ((info = g_file_enumerator_next_file(e, NULL, NULL))) {
            const char *n = g_file_info_get_name(info);
            g_autofree char *n_no_ext = g_strndup(n, strlen(n) - 5); // strip .toml or .css ideally, simplistic
            if (strstr(n, ".toml")) gtk_string_list_append(theme_mod, n_no_ext);
            g_object_unref(info);
        }
    }
    adw_combo_row_set_model(ui_theme, G_LIST_MODEL(theme_mod));
    int th_idx = 0;
    for(guint i=0; i<g_list_model_get_n_items(G_LIST_MODEL(theme_mod)); ++i){
        if (g_strcmp0(gtk_string_list_get_string(theme_mod, i), w_theme)==0) th_idx = i;
    }
    adw_combo_row_set_selected(ui_theme, th_idx);
    adw_preferences_group_add(st_group, GTK_WIDGET(ui_theme));

    ui_term = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_term), "Terminal Emulator");
    gtk_editable_set_text(GTK_EDITABLE(ui_term), w_terminal ? w_terminal : "");
    adw_preferences_group_add(st_group, GTK_WIDGET(ui_term));

    ui_srch_ph = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_srch_ph), "Search Placeholder");
    gtk_editable_set_text(GTK_EDITABLE(ui_srch_ph), w_placeholder ? w_placeholder : "");
    adw_preferences_group_add(st_group, GTK_WIDGET(ui_srch_ph));

    ui_srch_t_head = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_srch_t_head), "Typeahead");
    adw_switch_row_set_active(ui_srch_t_head, w_typeahead);
    adw_preferences_group_add(st_group, GTK_WIDGET(ui_srch_t_head));

    ui_srch_init = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ui_srch_init), "Show Initial Entries");
    adw_switch_row_set_active(ui_srch_init, w_show_initial);
    adw_preferences_group_add(st_group, GTK_WIDGET(ui_srch_init));

    /* ── Plugins ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *pl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(pl_group, "Plugins");
    adw_preferences_page_add(page, pl_group);

    s_apps = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(s_apps), "Applications");
    adw_switch_row_set_active(s_apps, p_apps);
    adw_preferences_group_add(pl_group, GTK_WIDGET(s_apps));

    s_run = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(s_run), "Runner (Custom Scripts)");
    adw_switch_row_set_active(s_run, p_runner);
    adw_preferences_group_add(pl_group, GTK_WIDGET(s_run));

    s_cmd = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(s_cmd), "Commands (Settings, Sleep, etc.)");
    adw_switch_row_set_active(s_cmd, p_commands);
    adw_preferences_group_add(pl_group, GTK_WIDGET(s_cmd));

    s_ssh = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(s_ssh), "SSH Hosts");
    adw_switch_row_set_active(s_ssh, p_ssh);
    adw_preferences_group_add(pl_group, GTK_WIDGET(s_ssh));

    return overlay;
}
