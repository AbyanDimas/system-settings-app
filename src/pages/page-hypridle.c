#include "page-hypridle.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int timeout;
    char *on_timeout;
    char *on_resume;
    
    AdwExpanderRow *ui_row;
    AdwSpinRow *ui_spin;
    AdwEntryRow *ui_on_timeout;
    AdwEntryRow *ui_on_resume;
} HypridleListener;

static GList *listeners = NULL;
static char *general_block = NULL;
static AdwPreferencesGroup *listeners_group = NULL;
static GtkWidget *list_box = NULL;
static GtkWidget *page_widget = NULL;

static void free_listener(HypridleListener *l) {
    if (!l) return;
    g_free(l->on_timeout);
    g_free(l->on_resume);
    g_free(l);
}

static void parse_hypridle_conf(void) {
    g_list_free_full(listeners, (GDestroyNotify)free_listener);
    listeners = NULL;
    g_free(general_block);
    general_block = NULL;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "hypridle.conf", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) {
        return;
    }

    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    
    GString *gen_str = g_string_new("");
    gboolean in_general = FALSE;
    gboolean in_listener = FALSE;
    HypridleListener *cur_l = NULL;

    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        
        if (g_str_has_prefix(line, "general {")) {
            in_general = TRUE;
            g_string_append_printf(gen_str, "%s\n", line);
            g_free(line);
            continue;
        } else if (g_str_has_prefix(line, "listener {")) {
            in_listener = TRUE;
            cur_l = g_new0(HypridleListener, 1);
            cur_l->timeout = 300;
            cur_l->on_timeout = g_strdup("");
            cur_l->on_resume = g_strdup("");
            g_free(line);
            continue;
        } else if (g_strcmp0(line, "}") == 0) {
            if (in_general) {
                in_general = FALSE;
                g_string_append_printf(gen_str, "}\n\n");
            } else if (in_listener) {
                in_listener = FALSE;
                if (cur_l) {
                    listeners = g_list_append(listeners, cur_l);
                    cur_l = NULL;
                }
            }
            g_free(line);
            continue;
        }

        if (in_general) {
            g_string_append_printf(gen_str, "    %s\n", line);
        } else if (in_listener && cur_l) {
            if (g_str_has_prefix(line, "timeout")) {
                char *eq = strchr(line, '=');
                if (eq) cur_l->timeout = atoi(eq + 1);
            } else if (g_str_has_prefix(line, "on-timeout")) {
                char *eq = strchr(line, '=');
                if (eq) cur_l->on_timeout = g_strdup(g_strstrip(eq + 1));
            } else if (g_str_has_prefix(line, "on-resume")) {
                char *eq = strchr(line, '=');
                if (eq) cur_l->on_resume = g_strdup(g_strstrip(eq + 1));
            }
        } else if (strlen(line) > 0) {
            // Unrecognized root level strings
            g_string_append_printf(gen_str, "%s\n", line);
        }
        g_free(line);
    }
    
    general_block = g_string_free(gen_str, FALSE);
}

static void on_save_config(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "hypridle.conf", NULL);
    
    GString *out = g_string_new("");
    if (general_block) {
        g_string_append(out, general_block);
    }

    for (GList *iter = listeners; iter != NULL; iter = iter->next) {
        HypridleListener *l = iter->data;
        // update from UI
        if (l->ui_spin) {
            const char *text = gtk_editable_get_text(GTK_EDITABLE(l->ui_spin));
            if (text && strlen(text) > 0) {
                l->timeout = atoi(text);
            }
            // fallback to double if spin row doesn't have text
            l->timeout = (int)adw_spin_row_get_value(l->ui_spin);
        }
        
        if (l->ui_on_timeout) {
            g_free(l->on_timeout);
            l->on_timeout = g_strdup(gtk_editable_get_text(GTK_EDITABLE(l->ui_on_timeout)));
        }
        if (l->ui_on_resume) {
            g_free(l->on_resume);
            l->on_resume = g_strdup(gtk_editable_get_text(GTK_EDITABLE(l->ui_on_resume)));
        }

        g_string_append_printf(out, "listener {\n");
        g_string_append_printf(out, "    timeout = %d\n", l->timeout);
        if (l->on_timeout && strlen(l->on_timeout) > 0) {
            g_string_append_printf(out, "    on-timeout = %s\n", l->on_timeout);
        }
        if (l->on_resume && strlen(l->on_resume) > 0) {
            g_string_append_printf(out, "    on-resume = %s\n", l->on_resume);
        }
        g_string_append_printf(out, "}\n\n");
    }

    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    // Reload hypridle
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("killall hypridle; hypridle &", &err);

    // Show toast
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Hypridle configuration saved and reloaded."));
        }
    }
}

static void on_delete_listener(GtkButton *btn, gpointer user_data) {
    (void)btn;
    HypridleListener *l = user_data;
    listeners = g_list_remove(listeners, l);
    if (l->ui_row && list_box) {
        gtk_list_box_remove(GTK_LIST_BOX(list_box), GTK_WIDGET(l->ui_row));
    }
    free_listener(l);
}

static void add_listener_ui(HypridleListener *l) {
    AdwExpanderRow *row = ADW_EXPANDER_ROW(adw_expander_row_new());
    g_autofree char *title = g_strdup_printf("Listener (Timeout: %ds)", l->timeout);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    
    l->ui_row = row;

    AdwSpinRow *spin = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 86400, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(spin), "Timeout (seconds)");
    adw_spin_row_set_value(spin, l->timeout);
    adw_expander_row_add_row(row, GTK_WIDGET(spin));
    l->ui_spin = spin;

    AdwEntryRow *on_to = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(on_to), "On-timeout cmd");
    if (l->on_timeout) gtk_editable_set_text(GTK_EDITABLE(on_to), l->on_timeout);
    adw_expander_row_add_row(row, GTK_WIDGET(on_to));
    l->ui_on_timeout = on_to;

    AdwEntryRow *on_res = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(on_res), "On-resume cmd");
    if (l->on_resume) gtk_editable_set_text(GTK_EDITABLE(on_res), l->on_resume);
    adw_expander_row_add_row(row, GTK_WIDGET(on_res));
    l->ui_on_resume = on_res;

    // Delete button inside the expander
    AdwActionRow *del_row = ADW_ACTION_ROW(adw_action_row_new());
    GtkWidget *del_btn = gtk_button_new_with_label("Delete Listener");
    gtk_widget_add_css_class(del_btn, "destructive-action");
    gtk_widget_set_valign(del_btn, GTK_ALIGN_CENTER);
    g_signal_connect(del_btn, "clicked", G_CALLBACK(on_delete_listener), l);
    adw_action_row_add_suffix(del_row, del_btn);
    adw_expander_row_add_row(row, GTK_WIDGET(del_row));

    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
}

static void on_add_custom_listener(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    HypridleListener *l = g_new0(HypridleListener, 1);
    l->timeout = 300;
    l->on_timeout = g_strdup("");
    l->on_resume = g_strdup("");
    listeners = g_list_append(listeners, l);
    add_listener_ui(l);
}

static void on_add_preset(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *preset = user_data;
    HypridleListener *l = g_new0(HypridleListener, 1);
    l->on_resume = g_strdup("");
    if (g_strcmp0(preset, "lock") == 0) {
        l->timeout = 300;
        l->on_timeout = g_strdup("hyprlock");
    } else if (g_strcmp0(preset, "dpms") == 0) {
        l->timeout = 600;
        l->on_timeout = g_strdup("hyprctl dispatch dpms off");
        g_free(l->on_resume);
        l->on_resume = g_strdup("hyprctl dispatch dpms on");
    } else if (g_strcmp0(preset, "suspend") == 0) {
        l->timeout = 1800;
        l->on_timeout = g_strdup("systemctl suspend");
    }
    listeners = g_list_append(listeners, l);
    add_listener_ui(l);
}

GtkWidget *page_hypridle_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->idle_daemon, "none") == 0) {
        return omarchy_make_unavailable_page("Idle Daemon", "sudo pacman -S hypridle", "system-suspend-symbolic");
    }

    /* Wrap page in ToastOverlay to show 'Saved' toasts */
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Idle & Sleep");
    adw_preferences_page_set_icon_name(page, "system-suspend-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    // Parse existing
    parse_hypridle_conf();

    /* ── Top controls ──────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    adw_action_row_set_subtitle(save_row, "Save config and restart hypridle");
    GtkWidget *save_btn = gtk_button_new_with_label("Save & Apply");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_config), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Presets ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *presets_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(presets_group, "Presets");
    adw_preferences_page_add(page, presets_group);

    AdwActionRow *p1 = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(p1), "Lock screen after 5 min");
    GtkWidget *bp1 = gtk_button_new_with_label("Add");
    gtk_widget_set_valign(bp1, GTK_ALIGN_CENTER);
    g_signal_connect(bp1, "clicked", G_CALLBACK(on_add_preset), "lock");
    adw_action_row_add_suffix(p1, bp1);
    adw_preferences_group_add(presets_group, GTK_WIDGET(p1));

    AdwActionRow *p2 = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(p2), "Turn off screen after 10 min");
    GtkWidget *bp2 = gtk_button_new_with_label("Add");
    gtk_widget_set_valign(bp2, GTK_ALIGN_CENTER);
    g_signal_connect(bp2, "clicked", G_CALLBACK(on_add_preset), "dpms");
    adw_action_row_add_suffix(p2, bp2);
    adw_preferences_group_add(presets_group, GTK_WIDGET(p2));

    AdwActionRow *p3 = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(p3), "Suspend system after 30 min");
    GtkWidget *bp3 = gtk_button_new_with_label("Add");
    gtk_widget_set_valign(bp3, GTK_ALIGN_CENTER);
    g_signal_connect(bp3, "clicked", G_CALLBACK(on_add_preset), "suspend");
    adw_action_row_add_suffix(p3, bp3);
    adw_preferences_group_add(presets_group, GTK_WIDGET(p3));

    /* ── Listeners ─────────────────────────────────────────────────────────── */
    listeners_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(listeners_group, "Listeners");
    adw_preferences_page_add(page, listeners_group);

    list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(listeners_group, list_box);

    for (GList *iter = listeners; iter != NULL; iter = iter->next) {
        add_listener_ui(iter->data);
    }

    AdwActionRow *add_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(add_row), "Custom Listener");
    GtkWidget *add_btn = gtk_button_new_with_label("Add Listener");
    gtk_widget_set_valign(add_btn, GTK_ALIGN_CENTER);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_custom_listener), NULL);
    adw_action_row_add_suffix(add_row, add_btn);
    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(add_row));

    return overlay;
}
