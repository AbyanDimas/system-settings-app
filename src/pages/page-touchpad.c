#include "page-touchpad.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Touchpad & Mouse page ────────────────────────────────────────────────
 * Reads/writes hyprland.conf input section.
 * Changes are applied via `hyprctl keyword`.
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_tap_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(
        active ? "hyprctl keyword input:touchpad:tap-to-click true"
               : "hyprctl keyword input:touchpad:tap-to-click false", &err);
}

static void on_natural_scroll_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(
        active ? "hyprctl keyword input:natural_scroll true"
               : "hyprctl keyword input:natural_scroll false", &err);
}

static void on_disable_while_typing_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(
        active ? "hyprctl keyword input:touchpad:disable_while_typing true"
               : "hyprctl keyword input:touchpad:disable_while_typing false", &err);
}

/* Read a keyword value from hyprctl getoption */
static gboolean read_bool_option(const char *option) {
    g_autofree char *cmd = g_strdup_printf("hyprctl getoption %s -j", option);
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync(cmd, &err);
    if (!out) return FALSE;
    /* JSON: {"option":"...","int":1,...} */
    return strstr(out, "\"int\":1") != NULL || strstr(out, "\"int\": 1") != NULL;
}

GtkWidget *page_touchpad_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Touchpad & Mouse");
    adw_preferences_page_set_icon_name(page, "input-mouse-symbolic");

    /* ── Touchpad group ──────────────────────────────────────────────── */
    AdwPreferencesGroup *tp_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(tp_group, "Touchpad");
    adw_preferences_group_set_description(tp_group,
        "Changes are applied immediately via hyprctl.");
    adw_preferences_page_add(page, tp_group);

    /* Tap to click */
    gboolean tap = read_bool_option("input:touchpad:tap-to-click");
    AdwSwitchRow *tap_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tap_row), "Tap to Click");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(tap_row), "Single-tap acts as a click");
    adw_switch_row_set_active(tap_row, tap);
    g_signal_connect(tap_row, "notify::active", G_CALLBACK(on_tap_toggled), NULL);
    adw_preferences_group_add(tp_group, GTK_WIDGET(tap_row));

    /* Natural scroll */
    gboolean nat = read_bool_option("input:natural_scroll");
    AdwSwitchRow *nat_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(nat_row), "Natural Scrolling");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(nat_row), "Scroll direction matches finger movement");
    adw_switch_row_set_active(nat_row, nat);
    g_signal_connect(nat_row, "notify::active", G_CALLBACK(on_natural_scroll_toggled), NULL);
    adw_preferences_group_add(tp_group, GTK_WIDGET(nat_row));

    /* Disable while typing */
    gboolean dwt = read_bool_option("input:touchpad:disable_while_typing");
    AdwSwitchRow *dwt_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dwt_row), "Disable While Typing");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(dwt_row), "Prevents accidental cursor movement");
    adw_switch_row_set_active(dwt_row, dwt);
    g_signal_connect(dwt_row, "notify::active", G_CALLBACK(on_disable_while_typing_toggled), NULL);
    adw_preferences_group_add(tp_group, GTK_WIDGET(dwt_row));

    /* ── Mouse group ─────────────────────────────────────────────────── */
    AdwPreferencesGroup *mouse_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(mouse_group, "Mouse");
    adw_preferences_page_add(page, mouse_group);

    AdwActionRow *sens_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sens_row), "Sensitivity");
    adw_action_row_set_subtitle(sens_row, "Adjust in ~/.config/hypr/hyprland.conf");
    adw_preferences_group_add(mouse_group, GTK_WIDGET(sens_row));

    return GTK_WIDGET(page);
}
