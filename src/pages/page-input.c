#include "page-input.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *kb_layout;
    char *kb_variant;
    char *kb_options;
    int repeat_rate;
    int repeat_delay;
    gboolean numlock_by_default;

    double sensitivity;
    char *accel_profile;
    gboolean left_handed;
    char *scroll_method;

    gboolean natural_scroll;
    gboolean tap_to_click;
    gboolean disable_while_typing;
    double scroll_factor;
} HyprInputConf;

static HyprInputConf in_cfg;
static char *general_block = NULL;

static GtkWidget *page_widget = NULL;

static AdwEntryRow *r_kb_layout, *r_kb_variant, *r_kb_options;
static AdwSpinRow *r_repeat_rate, *r_repeat_delay;
static AdwSwitchRow *r_numlock;

static AdwSpinRow *r_sens; // Wait GtkScale or AdwSpinRow doesn't support double out of box easily with precision unless digits set.
static AdwComboRow *r_accel_profile, *r_scroll_method;
static AdwSwitchRow *r_left_handed;

static AdwSwitchRow *r_nat_scroll, *r_tap_click, *r_dis_typing;
static AdwSpinRow *r_scroll_factor;

static void parse_input_conf(void) {
    g_free(in_cfg.kb_layout); in_cfg.kb_layout = g_strdup("us");
    g_free(in_cfg.kb_variant); in_cfg.kb_variant = g_strdup("");
    g_free(in_cfg.kb_options); in_cfg.kb_options = g_strdup("");
    in_cfg.repeat_rate = 25;
    in_cfg.repeat_delay = 600;
    in_cfg.numlock_by_default = FALSE;
    in_cfg.sensitivity = 0.0;
    g_free(in_cfg.accel_profile); in_cfg.accel_profile = g_strdup("adaptive");
    in_cfg.left_handed = FALSE;
    g_free(in_cfg.scroll_method); in_cfg.scroll_method = g_strdup("");
    in_cfg.natural_scroll = FALSE;
    in_cfg.tap_to_click = TRUE;
    in_cfg.disable_while_typing = TRUE;
    in_cfg.scroll_factor = 1.0;

    g_free(general_block);
    general_block = NULL;

    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "input.conf", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) return;

    g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
    GString *gen_str = g_string_new("");

    int in_input = 0;
    int in_touchpad = 0;

    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));

        if (g_str_has_prefix(line, "input {")) {
            in_input = 1; g_free(line); continue;
        } else if (g_str_has_prefix(line, "touchpad {")) {
            in_touchpad = 1; g_free(line); continue;
        } else if (g_strcmp0(line, "}") == 0) {
            in_input = 0;
            in_touchpad = 0;
            g_free(line); continue;
        }

        if (in_touchpad) {
            char *eq = strchr(line, '=');
            if (eq) {
                char *k = g_strndup(line, eq - line); g_strstrip(k);
                char *v = g_strdup(eq + 1); g_strstrip(v);
                if (g_strcmp0(k, "natural_scroll") == 0) in_cfg.natural_scroll = (g_strcmp0(v, "true") == 0 || g_strcmp0(v, "yes") == 0 || g_strcmp0(v, "1") == 0);
                else if (g_strcmp0(k, "tap-to-click") == 0) in_cfg.tap_to_click = (g_strcmp0(v, "true") == 0 || g_strcmp0(v, "yes") == 0 || g_strcmp0(v, "1") == 0);
                else if (g_strcmp0(k, "disable_while_typing") == 0) in_cfg.disable_while_typing = (g_strcmp0(v, "true") == 0 || g_strcmp0(v, "yes") == 0 || g_strcmp0(v, "1") == 0);
                else if (g_strcmp0(k, "scroll_factor") == 0) in_cfg.scroll_factor = atof(v);
                g_free(k); g_free(v);
            }
        } else if (in_input) {
            char *eq = strchr(line, '=');
            if (eq) {
                char *k = g_strndup(line, eq - line); g_strstrip(k);
                char *v = g_strdup(eq + 1); g_strstrip(v);
                if (g_strcmp0(k, "kb_layout") == 0) { g_free(in_cfg.kb_layout); in_cfg.kb_layout = g_strdup(v); }
                else if (g_strcmp0(k, "kb_variant") == 0) { g_free(in_cfg.kb_variant); in_cfg.kb_variant = g_strdup(v); }
                else if (g_strcmp0(k, "kb_options") == 0) { g_free(in_cfg.kb_options); in_cfg.kb_options = g_strdup(v); }
                else if (g_strcmp0(k, "repeat_rate") == 0) in_cfg.repeat_rate = atoi(v);
                else if (g_strcmp0(k, "repeat_delay") == 0) in_cfg.repeat_delay = atoi(v);
                else if (g_strcmp0(k, "numlock_by_default") == 0) in_cfg.numlock_by_default = (g_strcmp0(v, "true") == 0 || g_strcmp0(v, "1") == 0);
                else if (g_strcmp0(k, "sensitivity") == 0) in_cfg.sensitivity = atof(v);
                else if (g_strcmp0(k, "accel_profile") == 0) { g_free(in_cfg.accel_profile); in_cfg.accel_profile = g_strdup(v); }
                else if (g_strcmp0(k, "left_handed") == 0) in_cfg.left_handed = (g_strcmp0(v, "true") == 0 || g_strcmp0(v, "1") == 0);
                else if (g_strcmp0(k, "scroll_method") == 0) { g_free(in_cfg.scroll_method); in_cfg.scroll_method = g_strdup(v); }
                g_free(k); g_free(v);
            }
        } else if (strlen(line) > 0) {
            g_string_append_printf(gen_str, "%s\n", line);
        }
        g_free(line);
    }
    general_block = g_string_free(gen_str, FALSE);
}

static void on_save_config(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "input.conf", NULL);
    
    GString *out = g_string_new("");
    if (general_block) g_string_append(out, general_block);

    g_string_append_printf(out, "input {\n");
    g_string_append_printf(out, "    kb_layout = %s\n", gtk_editable_get_text(GTK_EDITABLE(r_kb_layout)));
    g_string_append_printf(out, "    kb_variant = %s\n", gtk_editable_get_text(GTK_EDITABLE(r_kb_variant)));
    g_string_append_printf(out, "    kb_options = %s\n", gtk_editable_get_text(GTK_EDITABLE(r_kb_options)));
    g_string_append_printf(out, "    repeat_rate = %d\n", (int)adw_spin_row_get_value(r_repeat_rate));
    g_string_append_printf(out, "    repeat_delay = %d\n", (int)adw_spin_row_get_value(r_repeat_delay));
    g_string_append_printf(out, "    numlock_by_default = %s\n", adw_switch_row_get_active(r_numlock) ? "true" : "false");
    
    g_string_append_printf(out, "    sensitivity = %.2f\n", adw_spin_row_get_value(r_sens));

    guint sel_accel = adw_combo_row_get_selected(r_accel_profile);
    const char *accels[] = {"adaptive", "flat", "custom"};
    g_string_append_printf(out, "    accel_profile = %s\n", accels[sel_accel < 3 ? sel_accel : 0]);

    g_string_append_printf(out, "    left_handed = %s\n", adw_switch_row_get_active(r_left_handed) ? "true" : "false");

    guint sel_scroll = adw_combo_row_get_selected(r_scroll_method);
    const char *scrolls[] = {"", "2fg", "edge", "on_button_down"};
    g_string_append_printf(out, "    scroll_method = %s\n", scrolls[sel_scroll < 4 ? sel_scroll : 0]);

    g_string_append_printf(out, "\n    touchpad {\n");
    g_string_append_printf(out, "        natural_scroll = %s\n", adw_switch_row_get_active(r_nat_scroll) ? "true" : "false");
    g_string_append_printf(out, "        tap-to-click = %s\n", adw_switch_row_get_active(r_tap_click) ? "true" : "false");
    g_string_append_printf(out, "        disable_while_typing = %s\n", adw_switch_row_get_active(r_dis_typing) ? "true" : "false");
    g_string_append_printf(out, "        scroll_factor = %.2f\n", adw_spin_row_get_value(r_scroll_factor));
    g_string_append_printf(out, "    }\n");

    g_string_append_printf(out, "}\n");

    g_file_set_contents(path, out->str, -1, NULL);
    g_string_free(out, TRUE);

    g_autoptr(GError) err = NULL;
    subprocess_run_sync("hyprctl reload", &err);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Input settings saved"));
    }
}

GtkWidget *page_input_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->compositor, "hyprland") != 0) {
        return omarchy_make_unavailable_page("Hyprland Input", "This page only works natively under Hyprland", "input-keyboard-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Input & Peripherals");
    adw_preferences_page_set_icon_name(page, "input-keyboard-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    parse_input_conf();

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, ctrl_group);

    AdwActionRow *save_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(save_row), "Apply Changes");
    GtkWidget *save_btn = gtk_button_new_with_label("Save & Reload");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_widget_set_valign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_config), NULL);
    adw_action_row_add_suffix(save_row, save_btn);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(save_row));

    /* ── Keyboard ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *kb_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(kb_group, "Keyboard");
    adw_preferences_page_add(page, kb_group);

    r_kb_layout = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_kb_layout), "Layout (e.g. us)");
    gtk_editable_set_text(GTK_EDITABLE(r_kb_layout), in_cfg.kb_layout ? in_cfg.kb_layout : "us");
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_kb_layout));

    r_kb_variant = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_kb_variant), "Variant");
    gtk_editable_set_text(GTK_EDITABLE(r_kb_variant), in_cfg.kb_variant ? in_cfg.kb_variant : "");
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_kb_variant));

    r_kb_options = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_kb_options), "Options (e.g. ctrl:nocaps)");
    gtk_editable_set_text(GTK_EDITABLE(r_kb_options), in_cfg.kb_options ? in_cfg.kb_options : "");
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_kb_options));

    r_repeat_rate = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 100, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_repeat_rate), "Repeat Rate (chars/sec)");
    adw_spin_row_set_value(r_repeat_rate, in_cfg.repeat_rate);
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_repeat_rate));

    r_repeat_delay = ADW_SPIN_ROW(adw_spin_row_new_with_range(100, 2000, 50));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_repeat_delay), "Repeat Delay (ms)");
    adw_spin_row_set_value(r_repeat_delay, in_cfg.repeat_delay);
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_repeat_delay));

    r_numlock = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_numlock), "Numlock on Boot");
    adw_switch_row_set_active(r_numlock, in_cfg.numlock_by_default);
    adw_preferences_group_add(kb_group, GTK_WIDGET(r_numlock));

    /* ── Mouse ─────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *mouse_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(mouse_group, "Mouse");
    adw_preferences_page_add(page, mouse_group);

    r_sens = ADW_SPIN_ROW(adw_spin_row_new_with_range(-1.0, 1.0, 0.1));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(r_sens), 2);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_sens), "Sensitivity");
    adw_spin_row_set_value(r_sens, in_cfg.sensitivity);
    adw_preferences_group_add(mouse_group, GTK_WIDGET(r_sens));

    r_accel_profile = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_accel_profile), "Accel Profile");
    GtkStringList *accel_model = gtk_string_list_new(NULL);
    gtk_string_list_append(accel_model, "Adaptive");
    gtk_string_list_append(accel_model, "Flat");
    gtk_string_list_append(accel_model, "Custom");
    adw_combo_row_set_model(r_accel_profile, G_LIST_MODEL(accel_model));
    int a_idx = 0;
    if (g_strcmp0(in_cfg.accel_profile, "flat") == 0) a_idx = 1;
    else if (g_strcmp0(in_cfg.accel_profile, "custom") == 0) a_idx = 2;
    adw_combo_row_set_selected(r_accel_profile, a_idx);
    adw_preferences_group_add(mouse_group, GTK_WIDGET(r_accel_profile));

    r_left_handed = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_left_handed), "Left Handed");
    adw_switch_row_set_active(r_left_handed, in_cfg.left_handed);
    adw_preferences_group_add(mouse_group, GTK_WIDGET(r_left_handed));

    r_scroll_method = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_scroll_method), "Scroll Method");
    GtkStringList *sm_model = gtk_string_list_new(NULL);
    gtk_string_list_append(sm_model, "Default");
    gtk_string_list_append(sm_model, "2 Fingers");
    gtk_string_list_append(sm_model, "Edge");
    gtk_string_list_append(sm_model, "On Button Down");
    adw_combo_row_set_model(r_scroll_method, G_LIST_MODEL(sm_model));
    int s_idx = 0;
    if (g_strcmp0(in_cfg.scroll_method, "2fg") == 0) s_idx = 1;
    else if (g_strcmp0(in_cfg.scroll_method, "edge") == 0) s_idx = 2;
    else if (g_strcmp0(in_cfg.scroll_method, "on_button_down") == 0) s_idx = 3;
    adw_combo_row_set_selected(r_scroll_method, s_idx);
    adw_preferences_group_add(mouse_group, GTK_WIDGET(r_scroll_method));

    /* ── Touchpad ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *tp_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(tp_group, "Touchpad");
    adw_preferences_page_add(page, tp_group);

    r_nat_scroll = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_nat_scroll), "Natural Scroll");
    adw_switch_row_set_active(r_nat_scroll, in_cfg.natural_scroll);
    adw_preferences_group_add(tp_group, GTK_WIDGET(r_nat_scroll));

    r_tap_click = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_tap_click), "Tap to Click");
    adw_switch_row_set_active(r_tap_click, in_cfg.tap_to_click);
    adw_preferences_group_add(tp_group, GTK_WIDGET(r_tap_click));

    r_dis_typing = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_dis_typing), "Disable while Typing");
    adw_switch_row_set_active(r_dis_typing, in_cfg.disable_while_typing);
    adw_preferences_group_add(tp_group, GTK_WIDGET(r_dis_typing));

    r_scroll_factor = ADW_SPIN_ROW(adw_spin_row_new_with_range(0.1, 5.0, 0.1));
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(r_scroll_factor), 2);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_scroll_factor), "Scroll Factor");
    adw_spin_row_set_value(r_scroll_factor, in_cfg.scroll_factor);
    adw_preferences_group_add(tp_group, GTK_WIDGET(r_scroll_factor));

    return overlay;
}
