#include "page-wallpaper.h"
#include "../utils/subprocess.h"
#include <adwaita.h>
#include <gtk/gtk.h>

/* ── Wallpaper page ───────────────────────────────────────────────────────
 * Shows wallpapers from ~/Pictures and ~/.local/share/backgrounds.
 * Applies via `swww img <path>` (swww wallpaper daemon).
 * Falls back to `hyprctl dispatch exec swww-daemon` if not running.
 * ─────────────────────────────────────────────────────────────────────────*/

typedef struct {
    const char *path;
    GtkFlowBox *flowbox;
} WallpaperItem;

static void apply_wallpaper(const char *path) {
    /* Ensure swww-daemon is running */
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("swww-daemon &", &err);
    g_clear_error(&err);

    /* Apply wallpaper */
    g_autofree char *cmd = g_strdup_printf(
        "swww img \"%s\" --transition-type fade --transition-duration 0.5", path);
    subprocess_run_sync(cmd, &err);

    /* Fallback: swaybg */
    if (err) {
        g_clear_error(&err);
        g_autofree char *cmd2 = g_strdup_printf("swaybg -i \"%s\" &", path);
        subprocess_run_sync(cmd2, &err);
    }
}

static void on_wallpaper_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *path = (const char *)user_data;
    apply_wallpaper(path);
}

static void free_str_notify(gpointer data, GClosure *closure) {
    (void)closure;
    g_free(data);
}

static void add_wallpaper_button(GtkFlowBox *flowbox, const char *filepath) {
    GtkWidget *button = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(button), FALSE);
    gtk_widget_add_css_class(button, "wallpaper-card");
    gtk_widget_set_tooltip_text(button, filepath);

    /* Try to load thumbnail */
    GtkWidget *img = gtk_image_new_from_file(filepath);
    gtk_image_set_pixel_size(GTK_IMAGE(img), 160);
    gtk_widget_set_size_request(img, 160, 100);

    gtk_button_set_child(GTK_BUTTON(button), img);
    g_signal_connect_data(button, "clicked",
        G_CALLBACK(on_wallpaper_clicked),
        g_strdup(filepath), free_str_notify, 0);

    gtk_flow_box_append(flowbox, button);
}

static void scan_dir_for_wallpapers(GtkFlowBox *flowbox, const char *dir_path) {
    if (!g_file_test(dir_path, G_FILE_TEST_IS_DIR)) return;

    GError *err = NULL;
    GDir *dir = g_dir_open(dir_path, 0, &err);
    if (!dir) return;

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        /* Only images */
        if (!g_str_has_suffix(name, ".jpg") &&
            !g_str_has_suffix(name, ".jpeg") &&
            !g_str_has_suffix(name, ".png") &&
            !g_str_has_suffix(name, ".webp")) continue;

        g_autofree char *full = g_build_filename(dir_path, name, NULL);
        add_wallpaper_button(flowbox, full);
    }
    g_dir_close(dir);
}

GtkWidget *page_wallpaper_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Wallpaper");
    adw_preferences_page_set_icon_name(page, "preferences-desktop-wallpaper-symbolic");

    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Choose Wallpaper");
    adw_preferences_group_set_description(group,
        "Applied via swww. Place images in ~/Pictures or ~/.local/share/backgrounds.");
    adw_preferences_page_add(page, group);

    /* FlowBox for wallpaper grid */
    GtkFlowBox *flowbox = GTK_FLOW_BOX(gtk_flow_box_new());
    gtk_flow_box_set_max_children_per_line(flowbox, 4);
    gtk_flow_box_set_min_children_per_line(flowbox, 2);
    gtk_flow_box_set_selection_mode(flowbox, GTK_SELECTION_SINGLE);
    gtk_flow_box_set_row_spacing(flowbox, 8);
    gtk_flow_box_set_column_spacing(flowbox, 8);
    gtk_widget_set_margin_top(GTK_WIDGET(flowbox), 8);
    gtk_widget_set_margin_bottom(GTK_WIDGET(flowbox), 8);
    gtk_widget_set_margin_start(GTK_WIDGET(flowbox), 8);
    gtk_widget_set_margin_end(GTK_WIDGET(flowbox), 8);

    /* Scrolled window around the grid */
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(flowbox));
    adw_preferences_group_add(group, scroll);

    /* Scan wallpaper directories */
    g_autofree char *pictures = g_build_filename(g_get_home_dir(), "Pictures", NULL);
    g_autofree char *backgrounds = g_build_filename(
        g_get_home_dir(), ".local", "share", "backgrounds", NULL);
    g_autofree char *omarchy_walls = g_build_filename(
        g_get_home_dir(), ".local", "share", "omarchy", "wallpapers", NULL);

    scan_dir_for_wallpapers(flowbox, pictures);
    scan_dir_for_wallpapers(flowbox, backgrounds);
    scan_dir_for_wallpapers(flowbox, omarchy_walls);

    return GTK_WIDGET(page);
}
