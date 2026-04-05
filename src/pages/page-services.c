#include "page-services.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *list_group = NULL;
static GtkWidget *list_box = NULL;
static AdwSwitchRow *r_user_mode = NULL;

static void refresh_services(void);

static void run_cmd_async(gboolean is_user, const char *cmd, const char *svc) {
    g_autofree char *full = NULL;
    if (is_user) {
        full = g_strdup_printf("systemctl --user %s %s", cmd, svc);
    } else {
        full = g_strdup_printf("pkexec systemctl %s %s", cmd, svc);
    }
    
    g_autoptr(GError) err = NULL;
    subprocess_run_async(full, NULL, NULL);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            g_autofree char *msg = g_strdup_printf("Service action '%s' sent to %s", cmd, svc);
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
    
    // Refresh after a delay since systemd takes a second
    g_timeout_add(1000, (GSourceFunc)refresh_services, NULL);
}

typedef struct {
    char *svc;
    gboolean is_user;
} SvcArgs;

static void free_svc_args(gpointer data, GClosure *closure) {
    (void)closure;
    SvcArgs *args = data;
    if (args) {
        g_free(args->svc);
        g_free(args);
    }
}

static void on_svc_restart(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SvcArgs *args = user_data;
    run_cmd_async(args->is_user, "restart", args->svc);
}

static void on_svc_stop(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SvcArgs *args = user_data;
    run_cmd_async(args->is_user, "stop", args->svc);
}

static void on_svc_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    SvcArgs *args = user_data;
    gboolean active = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
    if (active) run_cmd_async(args->is_user, "enable", args->svc);
    else run_cmd_async(args->is_user, "disable", args->svc);
}

static void on_mode_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    refresh_services();
}

static void refresh_services(void) {
    if (!list_box) return;

    gtk_list_box_remove_all(GTK_LIST_BOX(list_box));

    gboolean is_user = adw_switch_row_get_active(r_user_mode);
    adw_preferences_group_set_title(list_group, is_user ? "Running User Services" : "Running System Services");

    g_autoptr(GError) err = NULL;
    const char *cmd = is_user ? 
        "systemctl --user list-units --type=service --state=running --no-pager --no-legend" : 
        "systemctl list-units --type=service --state=running --no-pager --no-legend";
    
    g_autofree char *out = subprocess_run_sync(cmd, &err);
    if (!out) return;

    g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        char *line = g_strstrip(g_strdup(lines[i]));
        if (strlen(line) == 0) { g_free(line); continue; }

        // line: "acpid.service     loaded active running ACPI event daemon"
        char *space = strchr(line, ' ');
        if (!space) { g_free(line); continue; }
        *space = '\0';
        
        char *svc = g_strdup(line);
        char *rest = space + 1;
        while (*rest == ' ' || *rest == '\t') rest++;
        
        // skip loaded, active, running
        for (int k=0; k<3; k++) {
            char *nspace = strchr(rest, ' ');
            if (nspace) {
                rest = nspace + 1;
                while (*rest == ' ' || *rest == '\t') rest++;
            }
        }
        char *desc = g_strdup(rest);

        AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), svc);
        adw_expander_row_set_subtitle(exp, desc);

        GtkWidget *ic = gtk_image_new_from_icon_name(is_user ? "user-info-symbolic" : "applications-system-symbolic");
        adw_expander_row_add_prefix(exp, ic);

        /* ── Controls inside Expander ────────────────────────────── */
        
        // Autostart Toggle
        AdwSwitchRow *sw = ADW_SWITCH_ROW(adw_switch_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sw), "Start on Boot");
        
        g_autofree char *ecmd = g_strdup_printf(is_user ? "systemctl --user is-enabled %s" : "systemctl is-enabled %s", svc);
        g_autofree char *eout = subprocess_run_sync(ecmd, &err);
        adw_switch_row_set_active(sw, (eout && strstr(eout, "enabled")));
        
        SvcArgs *args_en = g_new0(SvcArgs, 1);
        args_en->svc = g_strdup(svc);
        args_en->is_user = is_user;
        g_signal_connect_data(sw, "notify::active", G_CALLBACK(on_svc_enable_toggled), args_en, free_svc_args, 0);
        adw_expander_row_add_row(exp, GTK_WIDGET(sw));

        // Restart Button
        AdwActionRow *ac_res = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_res), "Restart Service");
        GtkWidget *b_res = gtk_button_new_from_icon_name("view-refresh-symbolic");
        gtk_widget_set_valign(b_res, GTK_ALIGN_CENTER);
        SvcArgs *args_res = g_new0(SvcArgs, 1);
        args_res->svc = g_strdup(svc);
        args_res->is_user = is_user;
        g_signal_connect_data(b_res, "clicked", G_CALLBACK(on_svc_restart), args_res, free_svc_args, 0);
        adw_action_row_add_suffix(ac_res, b_res);
        adw_expander_row_add_row(exp, GTK_WIDGET(ac_res));

        // Stop Button
        AdwActionRow *ac_stp = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_stp), "Stop Service");
        GtkWidget *b_stp = gtk_button_new_from_icon_name("process-stop-symbolic");
        gtk_widget_add_css_class(b_stp, "destructive-action");
        gtk_widget_set_valign(b_stp, GTK_ALIGN_CENTER);
        SvcArgs *args_stp = g_new0(SvcArgs, 1);
        args_stp->svc = g_strdup(svc);
        args_stp->is_user = is_user;
        g_signal_connect_data(b_stp, "clicked", G_CALLBACK(on_svc_stop), args_stp, free_svc_args, 0);
        adw_action_row_add_suffix(ac_stp, b_stp);
        adw_expander_row_add_row(exp, GTK_WIDGET(b_stp));

        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(exp));

        g_free(desc);

        g_free(svc);
        g_free(line);
    }
}

GtkWidget *page_services_new(void) {
    if (!g_find_program_in_path("systemctl")) {
        return omarchy_make_unavailable_page("Services", "systemd is not available.", "applications-system-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Services");
    adw_preferences_page_set_icon_name(page, "applications-system-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    AdwPreferencesGroup *hdr_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(hdr_group, "Daemon Domain");
    adw_preferences_page_add(page, hdr_group);

    r_user_mode = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_user_mode), "User Mode (--user)");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(r_user_mode), "View services tied to the current user rather than system-wide");
    g_signal_connect(r_user_mode, "notify::active", G_CALLBACK(on_mode_toggled), NULL);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(r_user_mode));

    AdwActionRow *ref_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ref_row), "Refresh Running Daemons");
    GtkWidget *btn = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
    g_signal_connect(btn, "clicked", G_CALLBACK(refresh_services), NULL);
    adw_action_row_add_suffix(ref_row, btn);
    adw_preferences_group_add(hdr_group, GTK_WIDGET(ref_row));

    list_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, list_group);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "boxed-list");
    adw_preferences_group_add(list_group, list_box);

    refresh_services();

    return overlay;
}
