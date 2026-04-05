#include "page-displays.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>
#include <gio/gio.h>

/* ── Displays page ────────────────────────────────────────────────────────
 * Parses hyprctl monitors output to show connected displays.
 * Configures global GDK_SCALE in ~/.config/hypr/monitors.conf
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_apply_monitor_settings(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    AdwDialog *dialog = g_object_get_data(G_OBJECT(btn), "dialog");
    const char *monitor = g_object_get_data(G_OBJECT(btn), "monitor_name");
    AdwComboRow *res_row = g_object_get_data(G_OBJECT(btn), "res_row");
    AdwComboRow *hz_row = g_object_get_data(G_OBJECT(btn), "hz_row");
    AdwComboRow *scale_row = g_object_get_data(G_OBJECT(btn), "scale_row");

    GListModel *rm = adw_combo_row_get_model(res_row);
    guint r_sel = adw_combo_row_get_selected(res_row);
    const char *res = gtk_string_list_get_string(GTK_STRING_LIST(rm), r_sel);

    GListModel *hm = adw_combo_row_get_model(hz_row);
    guint h_sel = adw_combo_row_get_selected(hz_row);
    const char *hz = gtk_string_list_get_string(GTK_STRING_LIST(hm), h_sel);

    GListModel *sm = adw_combo_row_get_model(scale_row);
    guint s_sel = adw_combo_row_get_selected(scale_row);
    const char *scale = gtk_string_list_get_string(GTK_STRING_LIST(sm), s_sel);

    g_autofree char *cmd = g_strdup_printf("hyprctl keyword monitor %s,%s@%s,auto,%s", monitor, res, hz, scale);
    subprocess_run_async(cmd, NULL, NULL);

    /* Also append to monitors.conf so it persists slightly better */
    g_autofree char *save_cmd = g_strdup_printf("sh -c \"echo 'monitor=%s,%s@%s,auto,%s' >> ~/.config/hypr/monitors.conf\"", monitor, res, hz, scale);
    subprocess_run_async(save_cmd, NULL, NULL);

    adw_dialog_close(dialog);
}

static void on_monitor_activated(AdwActionRow *row, gpointer user_data) {
    (void)user_data;
    const char *monitor_name = g_object_get_data(G_OBJECT(row), "monitor_name");
    if (!monitor_name) return;

    AdwDialog *dialog = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dialog, monitor_name);
    adw_dialog_set_content_width(dialog, 360);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);

    /* Resolution */
    AdwComboRow *res_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(res_row), "Resolution");
    GtkStringList *res_list = gtk_string_list_new(NULL);
    const char *resolutions[] = {"3840x2160", "2880x1920", "2560x1600", "2560x1440", "1920x1200", "1920x1080", "1366x768", "1280x720", NULL};
    for (int i=0; resolutions[i]; i++) gtk_string_list_append(res_list, resolutions[i]);
    adw_combo_row_set_model(res_row, G_LIST_MODEL(res_list));
    
    // Preset resolution
    const char *c_res = g_object_get_data(G_OBJECT(row), "current_res");
    adw_combo_row_set_selected(res_row, 5); /* Default 1080p */
    if (c_res) {
        for (int i=0; resolutions[i]; i++) {
            if (g_str_has_prefix(c_res, resolutions[i])) {
                adw_combo_row_set_selected(res_row, i);
                break;
            }
        }
    }

    /* Refresh Rate */
    AdwComboRow *hz_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(hz_row), "Refresh Rate");
    GtkStringList *hz_list = gtk_string_list_new(NULL);
    const char *hzs[] = {"60", "75", "90", "120", "144", "165", "240", NULL};
    for (int i=0; hzs[i]; i++) gtk_string_list_append(hz_list, hzs[i]);
    adw_combo_row_set_model(hz_row, G_LIST_MODEL(hz_list));

    // Preset HZ
    if (c_res) {
        char *at = strchr(c_res, '@');
        if (at) {
            double hz_val = atof(at + 1);
            for (int i=0; hzs[i]; i++) {
                if (abs((int)hz_val - atoi(hzs[i])) <= 1) { // Fuzzy match 59.94 to 60
                    adw_combo_row_set_selected(hz_row, i);
                    break;
                }
            }
        }
    }

    /* Scale */
    AdwComboRow *scale_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scale_row), "Display Scale (Wayland)");
    GtkStringList *slist = gtk_string_list_new(NULL);
    const char *scales[] = {"0.5", "0.8", "1", "1.17", "1.25", "1.33", "1.5", "2", NULL};
    for (int i=0; scales[i]; i++) gtk_string_list_append(slist, scales[i]);
    adw_combo_row_set_model(scale_row, G_LIST_MODEL(slist));
    adw_combo_row_set_selected(scale_row, 2); // default 1

    const char *c_scale = g_object_get_data(G_OBJECT(row), "current_scale");
    if (c_scale) {
        for (int i=0; scales[i]; i++) {
            // Compare float value to avoid precision mismatches like "1.25" vs "1.25000"
            if (fabs(atof(c_scale) - atof(scales[i])) < 0.05) {
                adw_combo_row_set_selected(scale_row, i);
                break;
            }
        }
    }

    /* Wrap the ComboRows in a GtkListBox to make them clickable */
    GtkWidget *list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(res_row));
    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(hz_row));
    gtk_list_box_append(GTK_LIST_BOX(list_box), GTK_WIDGET(scale_row));

    gtk_box_append(GTK_BOX(vbox), list_box);

    /* Apply button */
    GtkWidget *apply_btn = gtk_button_new_with_label("Apply & Save");
    gtk_widget_add_css_class(apply_btn, "suggested-action");
    gtk_widget_set_margin_top(apply_btn, 12);
    
    g_object_set_data(G_OBJECT(apply_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(apply_btn), "monitor_name", (gpointer)monitor_name);
    g_object_set_data(G_OBJECT(apply_btn), "res_row", res_row);
    g_object_set_data(G_OBJECT(apply_btn), "hz_row", hz_row);
    g_object_set_data(G_OBJECT(apply_btn), "scale_row", scale_row);
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_apply_monitor_settings), NULL);

    gtk_box_append(GTK_BOX(vbox), apply_btn);
    adw_dialog_set_child(dialog, vbox);
    
    adw_dialog_present(dialog, GTK_WIDGET(row));
}


static void on_gdk_scale_apply(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = adw_combo_row_get_selected(row);
    const char *scale_val = "1";
    if (selected == 1) scale_val = "1.25"; /* Approximation for GDK which prefers integers, but Hyprland handles it sometimes */
    if (selected == 2) scale_val = "1.5";
    if (selected == 3) scale_val = "2";

    /* Write to ~/.config/hypr/monitors.conf */
    g_autoptr(GError) err = NULL;
    g_autofree char *sed_cmd = g_strdup_printf(
        "sed -i -E 's/^[[:space:]]*env[[:space:]]*=[[:space:]]*GDK_SCALE.*/env = GDK_SCALE,%s/' ~/.config/hypr/monitors.conf",
        scale_val);
    subprocess_run_sync(sed_cmd, &err);
}

static guint read_gdk_scale(void) {
    g_autofree char *path = g_build_filename(g_get_home_dir(), ".config", "hypr", "monitors.conf", NULL);
    g_autofree char *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            if (strstr(lines[i], "env") && strstr(lines[i], "GDK_SCALE")) {
                if (strstr(lines[i], ",2")) return 3;
                if (strstr(lines[i], ",1.5")) return 2;
                if (strstr(lines[i], ",1.25")) return 1;
                return 0; /* default 1 */
            }
        }
    }
    return 0;
}

static void on_displays_scan_complete(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(user_data);
    if (!source_object) return;
    GSubprocess *proc = G_SUBPROCESS(source_object);
    g_autoptr(GError) error = NULL;
    g_autofree char *stdout_buf = NULL;

    g_subprocess_communicate_utf8_finish(proc, res, &stdout_buf, NULL, &error);
    if (!stdout_buf || error) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No displays detected");
        adw_action_row_set_subtitle(row, "hyprctl not available");
        adw_preferences_group_add(group, GTK_WIDGET(row));
        return;
    }

    g_auto(GStrv) lines = g_strsplit(stdout_buf, "\n", -1);
    char *current_name = NULL;
    char *current_res = NULL;
    char *current_scale = NULL;

    for (int i = 0; lines[i] != NULL; i++) {
        const char *line = g_strstrip(lines[i]);
        if (g_str_has_prefix(line, "Monitor ")) {
            if (current_name) {
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), current_name);
                g_autofree char *sub = g_strdup_printf("%s | Scale: %s", current_res ? current_res : "", current_scale ? current_scale : "1.0");
                adw_action_row_set_subtitle(row, sub);
                adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("video-display-symbolic"));
                gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
                g_object_set_data_full(G_OBJECT(row), "monitor_name", g_strdup(current_name), g_free);
                g_object_set_data_full(G_OBJECT(row), "current_res", g_strdup(current_res), g_free);
                g_object_set_data_full(G_OBJECT(row), "current_scale", g_strdup(current_scale), g_free);
                g_signal_connect(row, "activated", G_CALLBACK(on_monitor_activated), NULL);
                adw_preferences_group_add(group, GTK_WIDGET(row));
                g_free(current_name);
                g_free(current_res);
                g_free(current_scale);
                current_res = NULL;
                current_scale = NULL;
            }
            char *paren = strchr(line, '(');
            int name_len = paren ? (int)(paren - line - 8) : (int)strlen(line) - 8;
            if (name_len > 0) {
                current_name = g_strndup(line + 8, name_len);
                g_strstrip(current_name);
            }
        } else if (current_name && strchr(line, 'x') && strchr(line, '@')) {
            g_free(current_res);
            current_res = g_strdup(line);
        } else if (current_name && g_str_has_prefix(line, "scale: ")) {
            g_free(current_scale);
            current_scale = g_strdup(line + 7);
        }
    }

    if (current_name) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), current_name);
        g_autofree char *sub = g_strdup_printf("%s | Scale: %s", current_res ? current_res : "", current_scale ? current_scale : "1.0");
        adw_action_row_set_subtitle(row, sub);
        adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("video-display-symbolic"));
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_object_set_data_full(G_OBJECT(row), "monitor_name", g_strdup(current_name), g_free);
        g_object_set_data_full(G_OBJECT(row), "current_res", g_strdup(current_res), g_free);
        g_object_set_data_full(G_OBJECT(row), "current_scale", g_strdup(current_scale), g_free);
        g_signal_connect(row, "activated", G_CALLBACK(on_monitor_activated), NULL);
        adw_preferences_group_add(group, GTK_WIDGET(row));
        g_free(current_name);
        g_free(current_res);
        g_free(current_scale);

    }
}

static void on_brightness_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double val = gtk_range_get_value(range);
    g_autofree char *cmd = g_strdup_printf("brightnessctl s %d%%", (int)val);
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_displays_new(void) {
    OmarchySystemCaps *caps = omarchy_get_system_caps();
    if (strcmp(caps->display_tool, "none") == 0) {
        return omarchy_make_unavailable_page("Display Configuration Tool", "sudo pacman -S wlr-randr", "video-display-symbolic");
    }

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Displays");
    adw_preferences_page_set_icon_name(page, "video-display-symbolic");

    /* ── Connected Displays ─────────────────────────────────────────── */
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Connected Displays");
    adw_preferences_page_add(page, group);

    if (strcmp(caps->display_tool, "hyprctl") == 0) {
        subprocess_run_async("hyprctl monitors", on_displays_scan_complete, group);
    } else {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Managed externally");
        g_autofree char *sub = g_strdup_printf("Tool available: %s", caps->display_tool);
        adw_action_row_set_subtitle(row, sub);
        adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("video-display-symbolic"));
        adw_preferences_group_add(group, GTK_WIDGET(row));
    }

    /* ── Global Scaling ─────────────────────────────────────────────── */
    if (strcmp(caps->display_tool, "hyprctl") == 0) {
        AdwPreferencesGroup *scale_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(scale_group, "Global Scaling (XWayland)");
        adw_preferences_page_add(page, scale_group);

        AdwComboRow *gdk_row = ADW_COMBO_ROW(adw_combo_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gdk_row), "GDK_SCALE");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(gdk_row), "Forces scaling for legacy GTK3/X11 apps. Re-login to apply.");
        
        GtkStringList *model = gtk_string_list_new(NULL);
        gtk_string_list_append(model, "Normal (1x)");
        gtk_string_list_append(model, "Large (1.25x)");
        gtk_string_list_append(model, "Larger (1.5x)");
        gtk_string_list_append(model, "Huge (2x)");
        adw_combo_row_set_model(gdk_row, G_LIST_MODEL(model));
        adw_combo_row_set_selected(gdk_row, read_gdk_scale());
        g_signal_connect(gdk_row, "notify::selected", G_CALLBACK(on_gdk_scale_apply), NULL);
        
        adw_preferences_group_add(scale_group, GTK_WIDGET(gdk_row));
    }

    /* ── Screen Brightness ─────────────────────────────────────────── */
    AdwPreferencesGroup *bright_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(bright_group, "Screen Brightness");
    adw_preferences_page_add(page, bright_group);

    AdwActionRow *bright_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bright_row), "Brightness");
    adw_action_row_set_subtitle(bright_row, "Requires 'brightnessctl'");
    
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 100, 1);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_widget_set_valign(scale, GTK_ALIGN_CENTER);
    
    g_autoptr(GError) b_err = NULL;
    g_autofree char *b_out = subprocess_run_sync("brightnessctl i", &b_err);
    double initial_bright = 50;
    if (b_out) {
        char *p = strchr(b_out, '(');
        if (p) initial_bright = atof(p + 1);
    }
    gtk_range_set_value(GTK_RANGE(scale), initial_bright);
    
    g_signal_connect(scale, "value-changed", G_CALLBACK(on_brightness_changed), NULL);
    adw_action_row_add_suffix(bright_row, scale);
    adw_preferences_group_add(bright_group, GTK_WIDGET(bright_row));

    return GTK_WIDGET(page);
}
