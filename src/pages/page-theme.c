#include "page-theme.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

static GtkWidget *page_widget = NULL;
static GtkWidget *theme_list = NULL;

static void on_apply_theme(GtkButton *btn, gpointer user_data) {
    const char *theme_name = user_data;
    g_autofree char *cmd = g_strdup_printf("omarchy-theme set %s", theme_name);
    g_autoptr(GError) err = NULL;
    
    // We execute async to not block UI
    subprocess_run_async(cmd, NULL, NULL);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) {
            g_autofree char *msg = g_strdup_printf("Applying theme: %s...", theme_name);
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
}

static void on_generate_dynamic(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    
    // Try matugen or pywal based on what's installed
    subprocess_run_async("if command -v matugen >/dev/null; then matugen image ~/.cache/current_wallpaper; elif command -v wal >/dev/null; then wal -i ~/.cache/current_wallpaper; fi", NULL, NULL);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Generating dynamic theme from wallpaper..."));
    }
}

static void on_save_custom(GtkButton *btn, gpointer user_data) {
    AdwDialog *dlg = ADW_DIALOG(user_data);
    GtkWidget *vbox = gtk_widget_get_first_child(adw_dialog_get_child(dlg));
    AdwEntryRow *r_name = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(gtk_widget_get_first_child(vbox)));
    AdwEntryRow *r_bg = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_name)));
    AdwEntryRow *r_fg = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_bg)));
    AdwEntryRow *r_acc = ADW_ENTRY_ROW(gtk_widget_get_next_sibling(GTK_WIDGET(r_fg)));
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(r_name));
    if (strlen(name) == 0) return;

    g_autofree char *dir = g_build_filename(g_get_home_dir(), ".local", "share", "omarchy", "themes", NULL);
    g_mkdir_with_parents(dir, 0755);
    g_autofree char *path = g_build_filename(dir, g_strdup_printf("%s.css", name), NULL);

    g_autofree char *css = g_strdup_printf(
        "@define-color background %s;\n"
        "@define-color foreground %s;\n"
        "@define-color accent %s;\n",
        gtk_editable_get_text(GTK_EDITABLE(r_bg)),
        gtk_editable_get_text(GTK_EDITABLE(r_fg)),
        gtk_editable_get_text(GTK_EDITABLE(r_acc))
    );

    g_file_set_contents(path, css, -1, NULL);
    adw_dialog_close(dlg);

    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Custom theme saved."));
    }
}

static void on_create_custom(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    AdwDialog *dlg = ADW_DIALOG(adw_dialog_new());
    adw_dialog_set_title(dlg, "Create Theme");
    adw_dialog_set_content_width(dlg, 400);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12); gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12); gtk_widget_set_margin_bottom(box, 12);

    GtkWidget *lbl = gtk_label_new("Define Custom Colors (Hex)");
    gtk_box_append(GTK_BOX(box), lbl);

    AdwEntryRow *r_name = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_name), "Theme Name");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_name));

    AdwEntryRow *r_bg = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_bg), "Background Color");
    gtk_editable_set_text(GTK_EDITABLE(r_bg), "#1e1e2e");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_bg));

    AdwEntryRow *r_fg = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_fg), "Foreground Color");
    gtk_editable_set_text(GTK_EDITABLE(r_fg), "#cdd6f4");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_fg));

    AdwEntryRow *r_acc = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_acc), "Accent Color");
    gtk_editable_set_text(GTK_EDITABLE(r_acc), "#cba6f7");
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(r_acc));

    GtkWidget *btn_save = gtk_button_new_with_label("Save Theme");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_custom), dlg);
    gtk_box_append(GTK_BOX(box), btn_save);

    adw_dialog_set_child(dlg, box);
    adw_dialog_present(dlg, page_widget);
}

static void load_themes() {
    g_autofree char *dir = g_build_filename(g_get_home_dir(), ".local", "share", "omarchy", "themes", NULL);
    g_autoptr(GFile) d = g_file_new_for_path(dir);
    g_autoptr(GFileEnumerator) e = g_file_enumerate_children(d, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, NULL);

    if (e) {
        GFileInfo *info;
        while ((info = g_file_enumerator_next_file(e, NULL, NULL))) {
            const char *name = g_file_info_get_name(info);
            g_autofree char *n_no_ext = g_strndup(name, strlen(name) - 4); // strip .css or .json

            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), n_no_ext);
            
            GtkWidget *btn = gtk_button_new_with_label("Apply");
            gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
            g_signal_connect(btn, "clicked", G_CALLBACK(on_apply_theme), g_strdup(n_no_ext));
            adw_action_row_add_suffix(row, btn);

            adw_preferences_group_add(ADW_PREFERENCES_GROUP(theme_list), GTK_WIDGET(row));
            g_object_unref(info);
        }
    } else {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No themes found in ~/.local/share/omarchy/themes/");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(theme_list), GTK_WIDGET(row));
    }
}

GtkWidget *page_theme_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Theme");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-theme-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Dynamic ───────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *dyn_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(dyn_group, "Dynamic Generation");
    adw_preferences_group_set_description(dyn_group, "Extract colors from the current wallpaper");
    adw_preferences_page_add(page, dyn_group);

    AdwActionRow *gen_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gen_row), "Generate Theme");
    GtkWidget *gen_btn = gtk_button_new_with_label("Generate");
    gtk_widget_add_css_class(gen_btn, "suggested-action");
    gtk_widget_set_valign(gen_btn, GTK_ALIGN_CENTER);
    g_signal_connect(gen_btn, "clicked", G_CALLBACK(on_generate_dynamic), NULL);
    adw_action_row_add_suffix(gen_row, gen_btn);
    adw_preferences_group_add(dyn_group, GTK_WIDGET(gen_row));

    /* ── Custom ────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *cus_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(cus_group, "Custom Theme");
    adw_preferences_page_add(page, cus_group);

    AdwActionRow *cus_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cus_row), "Create New Theme");
    GtkWidget *cus_btn = gtk_button_new_with_label("Create");
    gtk_widget_set_valign(cus_btn, GTK_ALIGN_CENTER);
    g_signal_connect(cus_btn, "clicked", G_CALLBACK(on_create_custom), NULL);
    adw_action_row_add_suffix(cus_row, cus_btn);
    adw_preferences_group_add(cus_group, GTK_WIDGET(cus_row));

    /* ── Library ───────────────────────────────────────────────────────────── */
    theme_list = GTK_WIDGET(adw_preferences_group_new());
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(theme_list), "Preset Themes");
    adw_preferences_page_add(page, ADW_PREFERENCES_GROUP(theme_list));

    load_themes();

    return overlay;
}
