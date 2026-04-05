#include "page-app-permissions.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;

static void on_perm_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    char **args = user_data; // [appid, perm_type]
    if (!args) return;
    const char *appid = args[0];
    const char *ptype = args[1];

    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
    g_autofree char *cmd = NULL;

    if (strcmp(ptype, "network") == 0) {
        cmd = g_strdup_printf("flatpak override --user %s=network %s", active ? "--share" : "--unshare", appid);
    } else if (strcmp(ptype, "wayland") == 0) {
        cmd = g_strdup_printf("flatpak override --user %s=wayland %s", active ? "--socket" : "--nosocket", appid);
    } else if (strcmp(ptype, "x11") == 0) {
        cmd = g_strdup_printf("flatpak override --user %s=x11 %s=fallback-x11 %s", active ? "--socket" : "--nosocket", active ? "--socket" : "--nosocket", appid);
    } else if (strcmp(ptype, "home") == 0) {
        cmd = g_strdup_printf("flatpak override --user %s=home %s", active ? "--filesystem" : "--nofilesystem", appid);
    }

    if (cmd) {
        g_autoptr(GError) err = NULL;
        subprocess_run_async(cmd, NULL, NULL);
    }
}

static void free_perm_args(gpointer data, GClosure *closure) {
    (void)closure;
    g_strfreev((char**)data);
}

// Toggles require args
static AdwSwitchRow *create_perm_toggle(const char *title, const char *subtitle, gboolean active, const char *appid, const char *ptype) {
    AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    if (subtitle) adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle);
    adw_switch_row_set_active(row, active);

    char **args = g_new0(char*, 3);
    args[0] = g_strdup(appid);
    args[1] = g_strdup(ptype);
    g_signal_connect_data(row, "notify::active", G_CALLBACK(on_perm_toggled), args, free_perm_args, 0);

    return row;
}

GtkWidget *page_app_permissions_new(void) {
    if (!g_find_program_in_path("flatpak")) {
        return omarchy_make_unavailable_page("App Permissions", "sudo pacman -S flatpak", "preferences-system-privacy-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "App Permissions");
    adw_preferences_page_set_icon_name(page, "preferences-system-privacy-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(grp, "Flatpak Applications");
    adw_preferences_group_set_description(grp, "Manage sandboxing permissions for installed graphical applications.");
    adw_preferences_page_add(page, grp);

    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync("flatpak list --app --columns=application,name", &err);

    if (out && strlen(out) > 0) {
        g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }

            char *tab = strchr(line, '\t');
            if (!tab) { g_free(line); continue; }
            *tab = '\0';
            
            char *appid = g_strstrip(g_strdup(line));
            char *name = g_strstrip(g_strdup(tab + 1));

            AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), name);
            adw_expander_row_set_subtitle(exp, appid);

            // Icon
            GtkWidget *ic = gtk_image_new_from_icon_name(appid);
            gtk_widget_set_size_request(ic, 32, 32);
            adw_expander_row_add_prefix(exp, ic);
            g_autofree char *pcmd = g_strdup_printf("flatpak info --show-permissions %s", appid);
            g_autofree char *pout = subprocess_run_sync(pcmd, &err);
            
            gboolean has_net = (pout && strstr(pout, "shared=network;") != NULL);
            gboolean has_wayl = (pout && strstr(pout, "wayland;") != NULL);
            gboolean has_x11 = (pout && (strstr(pout, "x11;") || strstr(pout, "fallback-x11;")));
            gboolean has_home = (pout && (strstr(pout, "home;") || strstr(pout, "host;")));

            adw_expander_row_add_row(exp, GTK_WIDGET(create_perm_toggle("Network Access", "Allow app to access the internet", has_net, appid, "network")));
            adw_expander_row_add_row(exp, GTK_WIDGET(create_perm_toggle("Wayland Windowing", "Native secure display server access", has_wayl, appid, "wayland")));
            adw_expander_row_add_row(exp, GTK_WIDGET(create_perm_toggle("X11 Windowing", "Legacy display server access", has_x11, appid, "x11")));
            adw_expander_row_add_row(exp, GTK_WIDGET(create_perm_toggle("Home Folder", "Read/write access to your user files", has_home, appid, "home")));

            adw_preferences_group_add(grp, GTK_WIDGET(exp));

            g_free(appid);
            g_free(name);
            g_free(line);
        }
    } else {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No Flatpak apps installed.");
        adw_preferences_group_add(grp, GTK_WIDGET(row));
    }

    return overlay;
}
