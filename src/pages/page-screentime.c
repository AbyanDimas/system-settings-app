#include "page-screentime.h"
#include "../utils/subprocess.h"
#include <adwaita.h>

/* ── Screen Time page ─────────────────────────────────────────────────────
 * Shows session uptime and last login as a basic screen time indicator.
 * ─────────────────────────────────────────────────────────────────────────*/

GtkWidget *page_screentime_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Screen Time");
    adw_preferences_page_set_icon_name(page, "preferences-system-time-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Session Info");
    adw_preferences_page_add(page, group);

    /* Uptime */
    g_autoptr(GError) err = NULL;
    g_autofree char *uptime = subprocess_run_sync("uptime -p", &err);
    if (uptime) g_strstrip(uptime);
    AdwActionRow *up_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(up_row), "System Uptime");
    adw_action_row_set_subtitle(up_row, uptime ? uptime : "Unknown");
    adw_preferences_group_add(group, GTK_WIDGET(up_row));

    /* Last login */
    g_autoptr(GError) err2 = NULL;
    g_autofree char *last = subprocess_run_sync("last -n 1 -w", &err2);
    if (last) g_strstrip(last);
    AdwActionRow *last_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(last_row), "Last Login");
    /* Just show first line */
    if (last) {
        char *nl = strchr(last, '\n');
        if (nl) *nl = '\0';
    }
    adw_action_row_set_subtitle(last_row, last ? last : "Unknown");
    adw_preferences_group_add(group, GTK_WIDGET(last_row));

    /* Who is logged in */
    g_autoptr(GError) err3 = NULL;
    g_autofree char *who = subprocess_run_sync("who", &err3);
    if (who) g_strstrip(who);
    if (who && strlen(who) > 0) {
        AdwActionRow *who_row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(who_row), "Active Sessions");
        char *nl = strchr(who, '\n');
        if (nl) *nl = '\0';
        adw_action_row_set_subtitle(who_row, who);
        adw_preferences_group_add(group, GTK_WIDGET(who_row));
    }

    return GTK_WIDGET(page);
}
