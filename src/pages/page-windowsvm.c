#include "page-windowsvm.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static AdwActionRow *sts_row = NULL;
static AdwEntryRow *r_ram = NULL;
static AdwSpinRow *r_cpu = NULL;
static AdwEntryRow *r_disk = NULL;

static gchar *get_compose_path(void) {
    return g_build_filename(g_get_home_dir(), ".config", "windows", "docker-compose.yml", NULL);
}

static void refresh_vm(void);

static void on_vm_action(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *action = user_data;
    g_autofree char *c_path = get_compose_path();
    
    if (strcmp(action, "start") == 0) {
        g_autofree char *cmd = g_strdup_printf("alacritty -t 'Starting Windows VM' -e bash -c 'docker compose -f \"%s\" up -d; read -p \"Done.\"'", c_path);
        subprocess_run_async(cmd, NULL, NULL);
    } else if (strcmp(action, "stop") == 0) {
        g_autofree char *cmd = g_strdup_printf("alacritty -t 'Stopping Windows VM' -e bash -c 'docker compose -f \"%s\" down; read -p \"Done.\"'", c_path);
        subprocess_run_async(cmd, NULL, NULL);
    } else if (strcmp(action, "rdp") == 0) {
        if (g_find_program_in_path("xfreerdp")) {
            subprocess_run_async("xfreerdp /v:localhost /u:docker /p:\"\"", NULL, NULL);
        } else {
            subprocess_run_async("xdg-open rdp://localhost", NULL, NULL);
        }
    }
    
    g_timeout_add(2000, (GSourceFunc)refresh_vm, NULL);
}

static char *extract_yaml_val(const char *contents, const char *key) {
    g_autofree char *search = g_strdup_printf("%s:", key);
    char *ptr = strstr(contents, search);
    if (!ptr) return NULL;
    ptr += strlen(search);
    while (*ptr == ' ' || *ptr == '"' || *ptr == '\'') ptr++;
    char *end = ptr;
    while (*end != '\n' && *end != '"' && *end != '\'' && *end != '\0') end++;
    return g_strndup(ptr, end - ptr);
}

static void on_save_alloc(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *c_path = get_compose_path();
    if (!g_file_test(c_path, G_FILE_TEST_EXISTS)) return;

    g_autofree char *contents = NULL;
    if (g_file_get_contents(c_path, &contents, NULL, NULL)) {
        const char *new_ram = gtk_editable_get_text(GTK_EDITABLE(r_ram));
        const char *new_disk = gtk_editable_get_text(GTK_EDITABLE(r_disk));
        int new_cpu = (int)adw_spin_row_get_value(r_cpu);

        g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
        GString *new_file = g_string_new(NULL);

        for (int i = 0; lines[i] != NULL; i++) {
            if (strstr(lines[i], "RAM_SIZE:")) {
                g_string_append_printf(new_file, "      RAM_SIZE: \"%s\"\n", new_ram);
            } else if (strstr(lines[i], "CPU_CORES:")) {
                g_string_append_printf(new_file, "      CPU_CORES: \"%d\"\n", new_cpu);
            } else if (strstr(lines[i], "DISK_SIZE:")) {
                g_string_append_printf(new_file, "      DISK_SIZE: \"%s\"\n", new_disk);
            } else {
                g_string_append(new_file, lines[i]);
                if (lines[i+1] != NULL) g_string_append_c(new_file, '\n');
            }
        }

        g_file_set_contents(c_path, new_file->str, new_file->len, NULL);
        g_string_free(new_file, TRUE);

        if (page_widget) {
            GtkWidget *overlay = gtk_widget_get_ancestor(page_widget, ADW_TYPE_TOAST_OVERLAY);
            if (overlay) adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new("VM Resources Updated. Restart VM to apply."));
        }
    }
}

static void refresh_vm(void) {
    if (!sts_row) return;

    g_autoptr(GError) err = NULL;
    g_autofree char *ps_out = subprocess_run_sync("docker ps | grep dockur/windows", &err);
    
    if (ps_out && strlen(ps_out) > 0) {
        adw_action_row_set_subtitle(sts_row, "Virtual Machine is Running (dockur/windows)");
    } else {
        adw_action_row_set_subtitle(sts_row, "Virtual Machine is Stopped");
    }
}

static void on_open_shared(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    g_autofree char *sh_path = g_build_filename(g_get_home_dir(), "Windows", NULL);
    g_mkdir_with_parents(sh_path, 0755);
    g_autofree char *cmd = g_strdup_printf("xdg-open \"%s\"", sh_path);
    subprocess_run_async(cmd, NULL, NULL);
}

GtkWidget *page_windowsvm_new(void) {
    g_autofree char *c_path = get_compose_path();
    if (!g_file_test(c_path, G_FILE_TEST_EXISTS)) {
        return omarchy_make_unavailable_page("Windows 11 VM", "Missing ~/.config/windows/docker-compose.yml", "video-display-symbolic");
    }

    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Windows Subsystem");
    adw_preferences_page_set_icon_name(page, "video-display-symbolic"); // Fallback
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Controls ──────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *ctrl_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(ctrl_group, "VM Status");
    adw_preferences_page_add(page, ctrl_group);

    sts_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sts_row), "Docker Instance Tracker");
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(sts_row));

    AdwActionRow *ac_b = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ac_b), "Lifecycle");
    GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    
    GtkWidget *bp = gtk_button_new_from_icon_name("media-playback-start-symbolic");
    gtk_widget_add_css_class(bp, "suggested-action");
    gtk_widget_set_valign(bp, GTK_ALIGN_CENTER);
    g_signal_connect(bp, "clicked", G_CALLBACK(on_vm_action), "start");
    gtk_box_append(GTK_BOX(bb), bp);

    GtkWidget *bs = gtk_button_new_from_icon_name("media-playback-stop-symbolic");
    gtk_widget_add_css_class(bs, "destructive-action");
    gtk_widget_set_valign(bs, GTK_ALIGN_CENTER);
    g_signal_connect(bs, "clicked", G_CALLBACK(on_vm_action), "stop");
    gtk_box_append(GTK_BOX(bb), bs);

    GtkWidget *br = gtk_button_new_with_label("Open Screen (RDP)");
    gtk_widget_set_valign(br, GTK_ALIGN_CENTER);
    g_signal_connect(br, "clicked", G_CALLBACK(on_vm_action), "rdp");
    gtk_box_append(GTK_BOX(bb), br);

    adw_action_row_add_suffix(ac_b, bb);
    adw_preferences_group_add(ctrl_group, GTK_WIDGET(ac_b));

    refresh_vm();

    /* ── Allocation ────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *res_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(res_group, "Hardware Targets");
    
    GtkWidget *sb = gtk_button_new_with_label("Apply Targets");
    gtk_widget_add_css_class(sb, "suggested-action");
    g_signal_connect(sb, "clicked", G_CALLBACK(on_save_alloc), NULL);
    adw_preferences_group_set_header_suffix(res_group, sb);
    adw_preferences_page_add(page, res_group);

    g_autofree char *contents = NULL;
    gboolean got = g_file_get_contents(c_path, &contents, NULL, NULL);

    r_ram = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_ram), "Memory Allocation (e.g. 8G)");
    if (got) {
        g_autofree char *v = extract_yaml_val(contents, "RAM_SIZE");
        if (v) gtk_editable_set_text(GTK_EDITABLE(r_ram), v);
    }
    adw_preferences_group_add(res_group, GTK_WIDGET(r_ram));

    r_cpu = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 128, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_cpu), "Processor Cores");
    if (got) {
        g_autofree char *v = extract_yaml_val(contents, "CPU_CORES");
        if (v) adw_spin_row_set_value(r_cpu, atof(v));
    }
    adw_preferences_group_add(res_group, GTK_WIDGET(r_cpu));

    r_disk = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_disk), "Virtual Disk Drive (e.g. 64G)");
    if (got) {
        g_autofree char *v = extract_yaml_val(contents, "DISK_SIZE");
        if (v) gtk_editable_set_text(GTK_EDITABLE(r_disk), v);
    }
    adw_preferences_group_add(res_group, GTK_WIDGET(r_disk));

    /* ── Shared ────────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *sh_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(sh_group, "Host Integration");
    adw_preferences_page_add(page, sh_group);

    AdwActionRow *sr = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr), "Shared Directory");
    adw_action_row_set_subtitle(sr, "Files placed in ~/Windows will appear inside the VM as a network drive");
    GtkWidget *bo = gtk_button_new_from_icon_name("folder-open-symbolic");
    gtk_widget_set_valign(bo, GTK_ALIGN_CENTER);
    g_signal_connect(bo, "clicked", G_CALLBACK(on_open_shared), NULL);
    adw_action_row_add_suffix(sr, bo);
    adw_preferences_group_add(sh_group, GTK_WIDGET(sr));

    return overlay;
}
