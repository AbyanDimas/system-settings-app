#include "page-appearance.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include <adwaita.h>
#include <gio/gio.h>
#include <unistd.h>

/* ── Appearance page ──────────────────────────────────────────────────────
 * Connects to Adwaita's StyleManager for dark/light mode and optionally
 * writes to gsettings for system-wide consistency.
 * Provides a picker for System themes.
 * ────────────────────────────────────────────────────────────────────────*/

static GtkStringList *theme_list_model = NULL;

static void on_dark_mode_toggled(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    gboolean active = adw_switch_row_get_active(row);
    AdwStyleManager *manager = adw_style_manager_get_default();

    if (active) {
        adw_style_manager_set_color_scheme(manager, ADW_COLOR_SCHEME_PREFER_DARK);
    } else {
        adw_style_manager_set_color_scheme(manager, ADW_COLOR_SCHEME_PREFER_LIGHT);
    }

    /* Also write to gsettings so other GTK apps pick it up */
    g_autofree char *cmd = g_strdup_printf(
        "gsettings set org.gnome.desktop.interface color-scheme %s",
        active ? "prefer-dark" : "prefer-light");
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_omarchy_theme_selected(AdwComboRow *row, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    guint selected = adw_combo_row_get_selected(row);
    if (selected == GTK_INVALID_LIST_POSITION) return;

    const char *theme_name = gtk_string_list_get_string(theme_list_model, selected);
    if (!theme_name) return;

    OmarchySystemCaps *caps = omarchy_get_system_caps();
    g_autoptr(GError) err = NULL;
    g_autofree char *cmd = NULL;

    if (strcmp(caps->compositor, "hyprland") == 0) {
        cmd = g_strdup_printf(
            "ln -fsn ~/.local/share/omarchy/themes/%s ~/.config/omarchy/current/theme && "
            "echo -n %s > ~/.config/omarchy/current/theme.name && "
            "hyprctl reload && pkill -SIGUSR2 waybar", theme_name, theme_name);
    } else {
        cmd = g_strdup_printf(
            "ln -fsn ~/.local/share/omarchy/themes/%s ~/.config/omarchy/current/theme && "
            "echo -n %s > ~/.config/omarchy/current/theme.name && "
            "pkill -SIGUSR2 waybar", theme_name, theme_name);
    }
    subprocess_run_async(cmd, NULL, NULL);
}

static void on_font_desc_changed(GtkFontDialogButton *btn, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    PangoFontDescription *desc = gtk_font_dialog_button_get_font_desc(btn);
    if (desc) {
        char *font_name = pango_font_description_to_string(desc);
        g_autofree char *cmd = g_strdup_printf("gsettings set org.gnome.desktop.interface font-name '%s'", font_name);
        g_free(font_name);
        subprocess_run_async(cmd, NULL, NULL); 
    }
}

GtkWidget *page_appearance_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Appearance");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-appearance-symbolic");

    /* ── Color Scheme group ──────────────────────────────────────────── */
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Color Scheme");
    adw_preferences_page_add(page, group);

    AdwStyleManager *manager = adw_style_manager_get_default();
    gboolean is_dark = adw_style_manager_get_dark(manager);

    AdwSwitchRow *dark_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dark_row), "Dark Mode");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(dark_row), "Switch between dark and light appearance");
    adw_switch_row_set_active(dark_row, is_dark);
    g_signal_connect(dark_row, "notify::active", G_CALLBACK(on_dark_mode_toggled), NULL);
    adw_preferences_group_add(group, GTK_WIDGET(dark_row));

    /* ── System Theme group ─────────────────────────────────────────── */
    AdwPreferencesGroup *om_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(om_group, "System Theme");
    adw_preferences_page_add(page, om_group);

    AdwComboRow *theme_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(theme_row), "Environment Theme");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(theme_row), "Applies global styling to Hyprland, Waybar, and other components.");
    
    theme_list_model = gtk_string_list_new(NULL);
    
    g_autofree char *dir_path = g_build_filename(g_get_home_dir(), ".local", "share", "omarchy", "themes", NULL);
    g_autoptr(GFile) dir = g_file_new_for_path(dir_path);
    g_autoptr(GFileEnumerator) enumerator = g_file_enumerate_children(dir,
        G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, NULL, NULL);

    guint current_theme_index = GTK_INVALID_LIST_POSITION;
    
    g_autofree char *name_file = g_build_filename(g_get_home_dir(), ".config", "omarchy", "current", "theme.name", NULL);
    g_autofree char *current_theme = NULL;
    g_file_get_contents(name_file, &current_theme, NULL, NULL);
    if (current_theme) g_strstrip(current_theme);

    if (enumerator) {
        GFileInfo *info;
        guint index = 0;
        GPtrArray *names = g_ptr_array_new_with_free_func(g_free);
        
        while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
            const char *name = g_file_info_get_name(info);
            g_ptr_array_add(names, g_strdup(name));
            g_object_unref(info);
        }
        
        /* Sort names alphabetically */
        g_ptr_array_sort(names, (GCompareFunc)g_strcmp0);
        
        for (guint i = 0; i < names->len; i++) {
            const char *name = g_ptr_array_index(names, i);
            gtk_string_list_append(theme_list_model, name);
            if (current_theme && g_strcmp0(name, current_theme) == 0) {
                current_theme_index = index;
            }
            index++;
        }
        g_ptr_array_unref(names);
    }
    
    adw_combo_row_set_model(theme_row, G_LIST_MODEL(theme_list_model));
    if (current_theme_index != GTK_INVALID_LIST_POSITION) {
        adw_combo_row_set_selected(theme_row, current_theme_index);
    }
    
    g_signal_connect(theme_row, "notify::selected", G_CALLBACK(on_omarchy_theme_selected), NULL);
    adw_preferences_group_add(om_group, GTK_WIDGET(theme_row));

    /* ── Font group ──────────────────────────────────────────────────── */
    AdwPreferencesGroup *font_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(font_group, "Interface Font");
    adw_preferences_group_set_description(font_group, "Set the font used across GTK applications.");
    adw_preferences_page_add(page, font_group);

    /* Read current font from gsettings */
    g_autoptr(GError) err2 = NULL;
    g_autofree char *current_font = subprocess_run_sync(
        "gsettings get org.gnome.desktop.interface font-name", &err2);
    if (current_font) g_strstrip(current_font);

    AdwActionRow *font_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(font_row), "Interface Font");
    adw_action_row_set_subtitle(font_row, "System-wide text styling");

    GtkFontDialog *dialog = gtk_font_dialog_new();
    GtkWidget *font_btn = gtk_font_dialog_button_new(dialog);
    gtk_widget_set_valign(font_btn, GTK_ALIGN_CENTER);

    if (current_font && strlen(current_font) > 2) {
        /* strip surrounding quotes */
        char *stripped = g_strdup(current_font + 1);
        stripped[strlen(stripped) - 1] = '\0';
        PangoFontDescription *desc = pango_font_description_from_string(stripped);
        if (desc) {
            gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(font_btn), desc);
            pango_font_description_free(desc);
        }
        g_free(stripped);
    }
    
    g_signal_connect(font_btn, "notify::font-desc", G_CALLBACK(on_font_desc_changed), NULL);
    adw_action_row_add_suffix(font_row, font_btn);
    adw_preferences_group_add(font_group, GTK_WIDGET(font_row));

    /* ── Icon theme group ────────────────────────────────────────────── */
    AdwPreferencesGroup *icon_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(icon_group, "Icon Theme");
    adw_preferences_page_add(page, icon_group);

    g_autoptr(GError) err3 = NULL;
    g_autofree char *current_icons = subprocess_run_sync(
        "gsettings get org.gnome.desktop.interface icon-theme", &err3);
    if (current_icons) g_strstrip(current_icons);

    AdwEntryRow *icon_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(icon_row), "Icon Theme");
    if (current_icons && strlen(current_icons) > 2) {
        char *stripped = g_strdup(current_icons + 1);
        stripped[strlen(stripped) - 1] = '\0';
        gtk_editable_set_text(GTK_EDITABLE(icon_row), stripped);
        g_free(stripped);
    }
    adw_preferences_group_add(icon_group, GTK_WIDGET(icon_row));

    return GTK_WIDGET(page);
}
