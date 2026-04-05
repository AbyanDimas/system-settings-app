#include "page-privacy-dashboard.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Removing static globals for widget lifecycle safety

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_cam_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec modprobe uvcvideo");
    } else {
        run_pkexec_async("pkexec modprobe -r uvcvideo");
    }
}

static void on_mic_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        subprocess_run_async("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ 0", NULL, NULL);
    } else {
        subprocess_run_async("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ 1", NULL, NULL);
    }
}

static void on_loc_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec systemctl enable --now geoclue.service");
    } else {
        run_pkexec_async("pkexec systemctl disable --now geoclue.service");
    }
}

static void refresh_usage(GtkWidget *list_box) {
    if (!list_box) return;
    
    // Clear list box using GTK4 standard
    GtkWidget *child = gtk_widget_get_first_child(list_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(list_box), child);
        child = next;
    }

    g_autoptr(GError) err = NULL;
    gboolean found = FALSE;

    // Camera Apps
    g_autofree char *f_out = subprocess_run_sync("fuser -v /dev/video* 2>/dev/null | awk 'NR>1 {print $NF}' | sort -u", &err);
    if (f_out) {
        g_auto(GStrv) lines = g_strsplit(f_out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) > 0) {
                found = TRUE;
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), line);
                adw_action_row_set_subtitle(row, "Accessing Camera");
                GtkWidget *ic = gtk_image_new_from_icon_name("camera-web-symbolic");
                gtk_widget_add_css_class(ic, "error");
                gtk_widget_set_size_request(ic, 24, 24);
                adw_action_row_add_prefix(row, ic);
                gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
            }
            g_free(line);
        }
    }

    // Audio recording apps
    g_autofree char *pw_out = subprocess_run_sync("pactl list source-outputs 2>/dev/null | grep 'application.name' | cut -d '\"' -f 2 | sort -u", &err);
    if (pw_out) {
        g_auto(GStrv) lines = g_strsplit(pw_out, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) > 0) {
                found = TRUE;
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), line);
                adw_action_row_set_subtitle(row, "Accessing Microphone");
                GtkWidget *ic = gtk_image_new_from_icon_name("audio-input-microphone-symbolic");
                gtk_widget_add_css_class(ic, "warning");
                gtk_widget_set_size_request(ic, 24, 24);
                adw_action_row_add_prefix(row, ic);
                gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
            }
            g_free(line);
        }
    }

    if (!found) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No active hardware access detected");
        gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(row));
    }
}

static gboolean on_auto_refresh(gpointer user_data) {
    if (!user_data) return G_SOURCE_REMOVE;
    refresh_usage(GTK_WIDGET(user_data));
    return G_SOURCE_CONTINUE;
}

static void remove_timeout_source(gpointer data) {
    guint id = GPOINTER_TO_UINT(data);
    if (id > 0) {
        g_source_remove(id);
    }
}

GtkWidget *page_privacy_dashboard_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Privacy Dashboard");
    adw_preferences_page_set_icon_name(page, "user-info-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;
    
    /* ── HW Toggles ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *hw_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(hw_group, "Hardware Killswitches");
    adw_preferences_page_add(page, hw_group);

    AdwSwitchRow *r_cam = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_cam), "Camera Access");
    g_autofree char *mod_out = subprocess_run_sync("lsmod | grep uvcvideo", &err);
    adw_switch_row_set_active(r_cam, (mod_out && strlen(mod_out) > 0));
    g_signal_connect(r_cam, "notify::active", G_CALLBACK(on_cam_toggled), NULL);
    adw_preferences_group_add(hw_group, GTK_WIDGET(r_cam));

    AdwSwitchRow *r_mic = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_mic), "Microphone Access");
    g_autofree char *mic_out = subprocess_run_sync("wpctl get-volume @DEFAULT_AUDIO_SOURCE@", &err);
    adw_switch_row_set_active(r_mic, (mic_out && !strstr(mic_out, "MUTED")));
    g_signal_connect(r_mic, "notify::active", G_CALLBACK(on_mic_toggled), NULL);
    adw_preferences_group_add(hw_group, GTK_WIDGET(r_mic));

    /* ── Location ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *loc_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(loc_group, "Location Services");
    adw_preferences_page_add(page, loc_group);

    AdwSwitchRow *r_loc = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_loc), "Allow apps to determine your location");
    g_autofree char *geo_out = subprocess_run_sync("systemctl is-active geoclue.service", &err);
    adw_switch_row_set_active(r_loc, (geo_out && strstr(geo_out, "active") && !strstr(geo_out, "inactive")));
    g_signal_connect(r_loc, "notify::active", G_CALLBACK(on_loc_toggled), NULL);
    adw_preferences_group_add(loc_group, GTK_WIDGET(r_loc));

    /* ── Live Usage ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *usage_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(usage_group, "Currently Accessing");
    
    GtkWidget *usage_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(usage_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(usage_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(usage_group, usage_list_box);
    adw_preferences_page_add(page, usage_group);

    refresh_usage(usage_list_box);
    guint id = g_timeout_add_seconds(5, on_auto_refresh, usage_list_box);
    g_object_set_data_full(G_OBJECT(overlay), "timer_id", GUINT_TO_POINTER(id), remove_timeout_source);

    return overlay;
}
