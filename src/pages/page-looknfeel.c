#include "page-looknfeel.h"
#include "../utils/subprocess.h"
#include <adwaita.h>
#include <stdlib.h>

/* ── Look & Feel page ─────────────────────────────────────────────────────
 * Controls UI rounding, gaps, animations for Hyprland.
 * Applies live via hyprctl keyword and writes to looknfeel.conf via sed.
 * ─────────────────────────────────────────────────────────────────────────*/

static void apply_and_save(const char *keyword, const char *conf_key, const char *value_str) {
    g_autoptr(GError) err = NULL;
    g_autofree char *live_cmd = g_strdup_printf("hyprctl keyword %s %s", keyword, value_str);
    subprocess_run_sync(live_cmd, &err);
    
    /* Persist replacing the specific line */
    g_autofree char *sed_cmd = g_strdup_printf(
        "sed -i -E 's/^[[:space:]]*#?[[:space:]]*%s[[:space:]]*=.*/    %s = %s/' ~/.config/hypr/looknfeel.conf",
        conf_key, conf_key, value_str);
    subprocess_run_sync(sed_cmd, NULL);
}

static void on_rounding_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    int val = (int)gtk_range_get_value(range);
    g_autofree char *vstr = g_strdup_printf("%d", val);
    apply_and_save("decoration:rounding", "rounding", vstr);
}

static void on_gaps_in_changed(GtkSpinButton *spin, gpointer user_data) {
    (void)user_data;
    int val = gtk_spin_button_get_value_as_int(spin);
    g_autofree char *vstr = g_strdup_printf("%d", val);
    apply_and_save("general:gaps_in", "gaps_in", vstr);
}

static void on_gaps_out_changed(GtkSpinButton *spin, gpointer user_data) {
    (void)user_data;
    int val = gtk_spin_button_get_value_as_int(spin);
    g_autofree char *vstr = g_strdup_printf("%d", val);
    apply_and_save("general:gaps_out", "gaps_out", vstr);
}

static void on_border_size_changed(GtkSpinButton *spin, gpointer user_data) {
    (void)user_data;
    int val = gtk_spin_button_get_value_as_int(spin);
    g_autofree char *vstr = g_strdup_printf("%d", val);
    apply_and_save("general:border_size", "border_size", vstr);
}

static void on_animations_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    apply_and_save("animations:enabled", "enabled", active ? "yes" : "no");
}

/* Helper to read integer values using hyprctl getoption */
static int read_int_option(const char *option, int default_val) {
    g_autofree char *cmd = g_strdup_printf("hyprctl getoption %s -j", option);
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync(cmd, &err);
    if (!out) return default_val;
    
    g_autofree char *search = g_strdup("\"int\": ");
    char *p = strstr(out, search);
    if (!p) {
        /* try without space */
        search = g_strdup("\"int\":");
        p = strstr(out, search);
        if (!p) return default_val;
    }
    p += strlen(search);
    return atoi(p);
}

GtkWidget *page_looknfeel_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Look and Feel");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-theme-symbolic");

    /* ── Window Decoration ─────────────────────────────────────────── */
    AdwPreferencesGroup *decor_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(decor_group, "Window Decoration");
    adw_preferences_page_add(page, decor_group);

    AdwActionRow *round_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(round_row), "Rounding");
    adw_action_row_set_subtitle(round_row, "Corner radius of windows");
    
    GtkWidget *round_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 24, 1);
    gtk_range_set_value(GTK_RANGE(round_scale), read_int_option("decoration:rounding", 12));
    gtk_scale_set_draw_value(GTK_SCALE(round_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(round_scale), GTK_POS_RIGHT);
    gtk_widget_set_valign(round_scale, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(round_scale, 200, -1);
    g_signal_connect(round_scale, "value-changed", G_CALLBACK(on_rounding_changed), NULL);
    adw_action_row_add_suffix(round_row, round_scale);
    adw_preferences_group_add(decor_group, GTK_WIDGET(round_row));

    /* ── Layout Group ───────────────────────────────────────────────── */
    AdwPreferencesGroup *layout_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(layout_group, "Layout and Borders");
    adw_preferences_page_add(page, layout_group);

    AdwActionRow *gaps_in_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gaps_in_row), "Inner Gaps");
    GtkWidget *spin_in = gtk_spin_button_new_with_range(0, 50, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_in), read_int_option("general:gaps_in", 2));
    gtk_widget_set_valign(spin_in, GTK_ALIGN_CENTER);
    g_signal_connect(spin_in, "value-changed", G_CALLBACK(on_gaps_in_changed), NULL);
    adw_action_row_add_suffix(gaps_in_row, spin_in);
    adw_preferences_group_add(layout_group, GTK_WIDGET(gaps_in_row));

    AdwActionRow *gaps_out_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gaps_out_row), "Outer Gaps");
    GtkWidget *spin_out = gtk_spin_button_new_with_range(0, 50, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_out), read_int_option("general:gaps_out", 4));
    gtk_widget_set_valign(spin_out, GTK_ALIGN_CENTER);
    g_signal_connect(spin_out, "value-changed", G_CALLBACK(on_gaps_out_changed), NULL);
    adw_action_row_add_suffix(gaps_out_row, spin_out);
    adw_preferences_group_add(layout_group, GTK_WIDGET(gaps_out_row));

    AdwActionRow *border_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(border_row), "Border Size");
    GtkWidget *spin_border = gtk_spin_button_new_with_range(0, 10, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_border), read_int_option("general:border_size", 2));
    gtk_widget_set_valign(spin_border, GTK_ALIGN_CENTER);
    g_signal_connect(spin_border, "value-changed", G_CALLBACK(on_border_size_changed), NULL);
    adw_action_row_add_suffix(border_row, spin_border);
    adw_preferences_group_add(layout_group, GTK_WIDGET(border_row));

    /* ── Effects Group ──────────────────────────────────────────────── */
    AdwPreferencesGroup *fx_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(fx_group, "Effects");
    adw_preferences_page_add(page, fx_group);

    AdwSwitchRow *anim_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(anim_row), "Animations");
    adw_switch_row_set_active(anim_row, read_int_option("animations:enabled", 1) == 1);
    g_signal_connect(anim_row, "notify::active", G_CALLBACK(on_animations_toggled), NULL);
    adw_preferences_group_add(fx_group, GTK_WIDGET(anim_row));

    return GTK_WIDGET(page);
}
