#include "page-sound.h"
#include "../window.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>
#include <stdlib.h>
#include <string.h>

static gboolean use_pactl = FALSE;

static void on_output_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    int vol = (int)gtk_range_get_value(range);
    g_autofree char *cmd = NULL;
    if (use_pactl) {
        cmd = g_strdup_printf("pactl set-sink-volume @DEFAULT_SINK@ %d%%", vol);
    } else {
        cmd = g_strdup_printf("wpctl set-volume @DEFAULT_AUDIO_SINK@ %d%%", vol);
    }
    g_spawn_command_line_async(cmd, NULL);
}

static void on_input_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    int vol = (int)gtk_range_get_value(range);
    g_autofree char *cmd = NULL;
    if (use_pactl) {
        cmd = g_strdup_printf("pactl set-source-volume @DEFAULT_SOURCE@ %d%%", vol);
    } else {
        cmd = g_strdup_printf("wpctl set-volume @DEFAULT_AUDIO_SOURCE@ %d%%", vol);
    }
    g_spawn_command_line_async(cmd, NULL);
}

static void on_mute_output_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean muted = adw_switch_row_get_active(row);
    g_autofree char *cmd = NULL;
    if (use_pactl) {
        cmd = g_strdup_printf("pactl set-sink-mute @DEFAULT_SINK@ %d", muted ? 1 : 0);
    } else {
        cmd = g_strdup_printf("wpctl set-mute @DEFAULT_AUDIO_SINK@ %d", muted ? 1 : 0);
    }
    g_spawn_command_line_async(cmd, NULL);
}

static void on_mute_input_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean muted = adw_switch_row_get_active(row);
    g_autofree char *cmd = NULL;
    if (use_pactl) {
        cmd = g_strdup_printf("pactl set-source-mute @DEFAULT_SOURCE@ %d", muted ? 1 : 0);
    } else {
        cmd = g_strdup_printf("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ %d", muted ? 1 : 0);
    }
    g_spawn_command_line_async(cmd, NULL);
}

static int parse_wpctl_volume(const char *output) {
    if (!output) return 50;
    const char *vol_str = strstr(output, "Volume: ");
    if (!vol_str) return 50;
    double vol = atof(vol_str + 8);
    return CLAMP((int)(vol * 100), 0, 100);
}

static gboolean parse_wpctl_muted(const char *output) {
    if (!output) return FALSE;
    return strstr(output, "[MUTED]") != NULL;
}

static int parse_pactl_volume(const char *output) {
    if (!output) return 50;
    const char *pct = strchr(output, '/');
    if (!pct) return 50;
    pct++; // past first slash
    while (*pct == ' ') pct++;
    int vol = atoi(pct);
    return CLAMP(vol, 0, 100);
}

static gboolean parse_pactl_muted(const char *output) {
    if (!output) return FALSE;
    return strstr(output, "yes") != NULL;
}

static void on_output_device_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = adw_combo_row_get_selected(row);
    GListModel *model = adw_combo_row_get_model(row);
    const char *dev_name = gtk_string_list_get_string(GTK_STRING_LIST(model), selected);
    if (!dev_name) return;

    g_autofree char *cmd = g_strdup_printf("pactl set-default-sink %s", dev_name);
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_input_device_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = adw_combo_row_get_selected(row);
    GListModel *model = adw_combo_row_get_model(row);
    const char *dev_name = gtk_string_list_get_string(GTK_STRING_LIST(model), selected);
    if (!dev_name) return;

    g_autofree char *cmd = g_strdup_printf("pactl set-default-source %s", dev_name);
    subprocess_run_async(cmd, NULL, NULL);
}

static AdwComboRow* create_device_combo(gboolean is_input) {
    g_autoptr(GError) err = NULL;
    g_autofree char *out = subprocess_run_sync(is_input ? "pactl list short sources" : "pactl list short sinks", &err);
    if (!out) return NULL;

    AdwComboRow *row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), is_input ? "Default Microphone" : "Default Audio Output");
    
    GtkStringList *model = gtk_string_list_new(NULL);
    g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
    
    int default_idx = 0;
    g_autofree char *def_out = subprocess_run_sync(is_input ? "pactl get-default-source" : "pactl get-default-sink", NULL);
    if (def_out) g_strstrip(def_out);

    int count = 0;
    for (int i = 0; lines[i] != NULL; i++) {
        if (strlen(lines[i]) < 2) continue;
        g_auto(GStrv) parts = g_strsplit(lines[i], "\t", -1);
        if (parts[0] && parts[1]) {
            char *name = g_strstrip(parts[1]);
            if (is_input && strstr(name, ".monitor")) continue;

            gtk_string_list_append(model, name);
            if (def_out && strcmp(def_out, name) == 0) {
                default_idx = count;
            }
            count++;
        }
    }

    adw_combo_row_set_model(row, G_LIST_MODEL(model));
    if (count > 0) adw_combo_row_set_selected(row, default_idx);

    g_signal_connect(row, "notify::selected", G_CALLBACK(is_input ? on_input_device_selected : on_output_device_selected), NULL);
    return row;
}


GtkWidget *page_sound_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    
    if (strcmp(caps->audio_backend, "none") == 0) {
        return omarchy_make_unavailable_page("Audio Daemon", "sudo pacman -S pipewire wireplumber", "audio-volume-muted-symbolic");
    }
    if (!caps->has_wpctl && !caps->has_pactl) {
        return omarchy_make_unavailable_page("Audio Control Tools (wpctl or pactl)", "sudo pacman -S wireplumber pulseaudio-utils", "audio-volume-muted-symbolic");
    }
    
    use_pactl = !caps->has_wpctl;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Sound");
    adw_preferences_page_set_icon_name(page, "audio-volume-high-symbolic");

    /* ── Output group ──────────────────────────────────────────────── */
    AdwPreferencesGroup *out_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(out_group, "Output");
    adw_preferences_page_add(page, out_group);

    if (caps->has_pactl) {
        AdwComboRow *sink_row = create_device_combo(FALSE);
        if (sink_row) adw_preferences_group_add(out_group, GTK_WIDGET(sink_row));
    }

    int current_vol = 50;

    gboolean muted = FALSE;
    
    if (use_pactl) {
        g_autofree char *v_out = subprocess_run_sync("pactl get-sink-volume @DEFAULT_SINK@", NULL);
        current_vol = parse_pactl_volume(v_out);
        g_autofree char *m_out = subprocess_run_sync("pactl get-sink-mute @DEFAULT_SINK@", NULL);
        muted = parse_pactl_muted(m_out);
    } else {
        g_autofree char *vol_out = subprocess_run_sync("wpctl get-volume @DEFAULT_AUDIO_SINK@", NULL);
        current_vol = parse_wpctl_volume(vol_out);
        muted = parse_wpctl_muted(vol_out);
    }

    AdwActionRow *vol_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(vol_row), "Output Volume");
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    gtk_range_set_value(GTK_RANGE(scale), current_vol);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    gtk_widget_set_valign(scale, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(scale, 200, -1);
    g_signal_connect(scale, "value-changed", G_CALLBACK(on_output_volume_changed), NULL);
    adw_action_row_add_suffix(vol_row, scale);
    adw_preferences_group_add(out_group, GTK_WIDGET(vol_row));

    AdwSwitchRow *mute_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mute_row), "Mute Output");
    adw_switch_row_set_active(mute_row, muted);
    g_signal_connect(mute_row, "notify::active", G_CALLBACK(on_mute_output_toggled), NULL);
    adw_preferences_group_add(out_group, GTK_WIDGET(mute_row));

    /* ── Input group ───────────────────────────────────────────────── */
    AdwPreferencesGroup *in_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(in_group, "Input (Microphone)");
    adw_preferences_page_add(page, in_group);

    if (caps->has_pactl) {
        AdwComboRow *src_row = create_device_combo(TRUE);
        if (src_row) adw_preferences_group_add(in_group, GTK_WIDGET(src_row));
    }

    int in_vol = 50;
    gboolean in_muted = FALSE;

    if (use_pactl) {
        g_autofree char *v_out = subprocess_run_sync("pactl get-source-volume @DEFAULT_SOURCE@", NULL);
        in_vol = parse_pactl_volume(v_out);
        g_autofree char *m_out = subprocess_run_sync("pactl get-source-mute @DEFAULT_SOURCE@", NULL);
        in_muted = parse_pactl_muted(m_out);
    } else {
        g_autofree char *in_vol_out = subprocess_run_sync("wpctl get-volume @DEFAULT_AUDIO_SOURCE@", NULL);
        in_vol = parse_wpctl_volume(in_vol_out);
        in_muted = parse_wpctl_muted(in_vol_out);
    }

    AdwActionRow *in_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_row), "Input Volume");
    GtkWidget *in_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    gtk_range_set_value(GTK_RANGE(in_scale), in_vol);
    gtk_scale_set_draw_value(GTK_SCALE(in_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(in_scale), GTK_POS_RIGHT);
    gtk_widget_set_valign(in_scale, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(in_scale, 200, -1);
    g_signal_connect(in_scale, "value-changed", G_CALLBACK(on_input_volume_changed), NULL);
    adw_action_row_add_suffix(in_row, in_scale);
    adw_preferences_group_add(in_group, GTK_WIDGET(in_row));

    AdwSwitchRow *in_mute_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(in_mute_row), "Mute Input");
    adw_switch_row_set_active(in_mute_row, in_muted);
    g_signal_connect(in_mute_row, "notify::active", G_CALLBACK(on_mute_input_toggled), NULL);
    adw_preferences_group_add(in_group, GTK_WIDGET(in_mute_row));

    return GTK_WIDGET(page);
}
