#include "page-docker.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwPreferencesGroup *cnt_group = NULL;
static AdwPreferencesGroup *img_group = NULL;
static GtkWidget *cnt_list_box = NULL;
static GtkWidget *img_list_box = NULL;
static AdwSwitchRow *r_enable = NULL;

static void refresh_docker(void);

static void run_pkexec_async(const char *cmd) {
    g_autoptr(GError) err = NULL;
    const char *argv[] = {"sh", "-c", cmd, NULL};
    GSubprocess *proc = g_subprocess_newv(argv, G_SUBPROCESS_FLAGS_NONE, &err);
    if (proc) {
        g_subprocess_wait_async(proc, NULL, NULL, NULL);
    }
}

static void on_enable_toggled(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec; (void)user_data;
    if (adw_switch_row_get_active(ADW_SWITCH_ROW(obj))) {
        run_pkexec_async("pkexec systemctl enable --now docker");
    } else {
        run_pkexec_async("pkexec systemctl disable --now docker");
    }
}

static void on_docker_btn(GtkButton *btn, gpointer user_data) {
    (void)btn;
    char **args = user_data; // [action, id]
    g_autofree char *cmd = g_strdup_printf("docker %s %s || pkexec docker %s %s", args[0], args[1], args[0], args[1]);
    
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
    refresh_docker();
}

static void free_docker_args(gpointer data, GClosure *closure) {
    (void)closure;
    g_strfreev((char**)data);
}

static void on_prune(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autoptr(GError) err = NULL;
    subprocess_run_sync("docker system prune -f -a || pkexec docker system prune -f -a", &err);
    if (page_widget) {
        GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
        if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("Pruned unused Docker containers, networks, and images"));
    }
    refresh_docker();
}

static void refresh_docker(void) {
    if (!cnt_list_box || !img_list_box) return;

    gtk_list_box_remove_all(GTK_LIST_BOX(cnt_list_box));
    gtk_list_box_remove_all(GTK_LIST_BOX(img_list_box));

    g_autoptr(GError) err = NULL;
    
    g_autofree char *sys_out = subprocess_run_sync("systemctl is-active docker", &err);
    gboolean is_active = (sys_out && strstr(sys_out, "active") && !strstr(sys_out, "inactive"));
    
    g_signal_handlers_block_by_func(r_enable, on_enable_toggled, NULL);
    adw_switch_row_set_active(r_enable, is_active);
    g_signal_handlers_unblock_by_func(r_enable, on_enable_toggled, NULL);

    if (!is_active) {
        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "Docker daemon is inactive. Cannot list containers.");
        gtk_list_box_append(GTK_LIST_BOX(cnt_list_box), GTK_WIDGET(row));
        return;
    }

    /* ── Containers ────────────────────────────────────────────── */
    g_autofree char *c_out = subprocess_run_sync("docker ps -a --format \"{{.ID}}|{{.Image}}|{{.Status}}|{{.Names}}\" || pkexec docker ps -a --format \"{{.ID}}|{{.Image}}|{{.Status}}|{{.Names}}\"", &err);
    if (c_out) {
        g_auto(GStrv) lines = g_strsplit(c_out, "\n", -1);
        int c = 0;
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }
            
            g_auto(GStrv) toks = g_strsplit(line, "|", 4);
            if (g_strv_length(toks) >= 4) {
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), toks[3]);
                adw_action_row_set_subtitle(row, g_strdup_printf("%s  ·  %s", toks[1], toks[2]));

                GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                
                gboolean running = strstr(toks[2], "Up ") != NULL;
                
                GtkWidget *bt = gtk_button_new_from_icon_name(running ? "media-playback-pause-symbolic" : "media-playback-start-symbolic");
                gtk_widget_set_valign(bt, GTK_ALIGN_CENTER);
                char **args_t = g_new0(char*, 3);
                args_t[0] = g_strdup(running ? "stop" : "start");
                args_t[1] = g_strdup(toks[0]);
                g_signal_connect_data(bt, "clicked", G_CALLBACK(on_docker_btn), args_t, free_docker_args, 0);
                gtk_box_append(GTK_BOX(bb), bt);

                GtkWidget *bd = gtk_button_new_from_icon_name("user-trash-symbolic");
                gtk_widget_add_css_class(bd, "destructive-action");
                gtk_widget_set_valign(bd, GTK_ALIGN_CENTER);
                char **args_d = g_new0(char*, 3);
                args_d[0] = g_strdup("rm -f");
                args_d[1] = g_strdup(toks[0]);
                g_signal_connect_data(bd, "clicked", G_CALLBACK(on_docker_btn), args_d, free_docker_args, 0);
                gtk_box_append(GTK_BOX(bb), bd);

                adw_action_row_add_suffix(row, bb);
                gtk_list_box_append(GTK_LIST_BOX(cnt_list_box), GTK_WIDGET(row));
                c++;
            }
            g_free(line);
        }
        if (c == 0) {
            AdwActionRow *r2 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r2), "No containers exist.");
            gtk_list_box_append(GTK_LIST_BOX(cnt_list_box), GTK_WIDGET(r2));
        }
    }

    /* ── Images ────────────────────────────────────────────────── */
    g_autofree char *i_out = subprocess_run_sync("docker images --format \"{{.Repository}}|{{.Tag}}|{{.Size}}\" || pkexec docker images --format \"{{.Repository}}|{{.Tag}}|{{.Size}}\"", &err);
    if (i_out) {
        g_auto(GStrv) lines = g_strsplit(i_out, "\n", -1);
        int c = 0;
        for (int i = 0; lines[i] != NULL; i++) {
            char *line = g_strstrip(g_strdup(lines[i]));
            if (strlen(line) == 0) { g_free(line); continue; }
            
            g_auto(GStrv) toks = g_strsplit(line, "|", 3);
            if (g_strv_length(toks) >= 3) {
                AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), g_strdup_printf("%s:%s", toks[0], toks[1]));
                adw_action_row_set_subtitle(row, g_strdup_printf("Size: %s", toks[2]));

                GtkWidget *bd = gtk_button_new_from_icon_name("user-trash-symbolic");
                gtk_widget_add_css_class(bd, "destructive-action");
                gtk_widget_set_valign(bd, GTK_ALIGN_CENTER);
                char **args_d = g_new0(char*, 3);
                args_d[0] = g_strdup("rmi");
                args_d[1] = g_strdup_printf("%s:%s", toks[0], toks[1]);
                g_signal_connect_data(bd, "clicked", G_CALLBACK(on_docker_btn), args_d, free_docker_args, 0);
                
                adw_action_row_add_suffix(row, bd);
                gtk_list_box_append(GTK_LIST_BOX(img_list_box), GTK_WIDGET(row));
                c++;
            }
            g_free(line);
        }
        if (c == 0) {
            AdwActionRow *r2 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r2), "No locally cached images.");
            gtk_list_box_append(GTK_LIST_BOX(img_list_box), GTK_WIDGET(r2));
        }
    }
}

GtkWidget *page_docker_new(void) {
    if (!g_find_program_in_path("docker")) {
        return omarchy_make_unavailable_page("Docker Management", "sudo pacman -S docker docker-compose", "applications-engineering-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Docker VM");
    adw_preferences_page_set_icon_name(page, "applications-engineering-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Daemon Status ─────────────────────────────────────────────────────── */
    AdwPreferencesGroup *st_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(st_group, "Daemon Configuration");
    adw_preferences_page_add(page, st_group);

    r_enable = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_enable), "Enable Docker Service");
    g_signal_connect(r_enable, "notify::active", G_CALLBACK(on_enable_toggled), NULL);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_enable));

    AdwActionRow *r_prune = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_prune), "System Prune");
    adw_action_row_set_subtitle(r_prune, "Delete all unused containers, networks, and images to free up disk space");
    GtkWidget *bp = gtk_button_new_with_label("Prune All");
    gtk_widget_add_css_class(bp, "destructive-action");
    gtk_widget_set_valign(bp, GTK_ALIGN_CENTER);
    g_signal_connect(bp, "clicked", G_CALLBACK(on_prune), NULL);
    adw_action_row_add_suffix(r_prune, bp);
    adw_preferences_group_add(st_group, GTK_WIDGET(r_prune));

    /* ── Instances ─────────────────────────────────────────────────────────── */
    cnt_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(cnt_group, "Running Containers");
    
    cnt_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(cnt_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(cnt_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(cnt_group, cnt_list_box);
    adw_preferences_page_add(page, cnt_group);

    img_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(img_group, "Cached Images");
    
    img_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(img_list_box, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(img_list_box), GTK_SELECTION_NONE);
    adw_preferences_group_add(img_group, img_list_box);
    adw_preferences_page_add(page, img_group);

    refresh_docker();

    return overlay;
}
