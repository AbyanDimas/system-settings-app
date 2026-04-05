#include "page-monitor.h"
#include "../utils/subprocess.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *page_widget = NULL;
static GtkProgressBar *pb_cpu = NULL;
static GtkProgressBar *pb_ram = NULL;
static GtkLabel *lbl_cpu = NULL;
static GtkLabel *lbl_ram = NULL;
static AdwPreferencesGroup *proc_group = NULL;
static GtkWidget *proc_list_box = NULL;
static guint timer_id = 0;

static void free_g_free(gpointer data, GClosure *closure) { (void)closure; g_free(data); }

static unsigned long long last_total_cpu = 0;
static unsigned long long last_idle_cpu = 0;

static void parse_proc_stat(double *usage) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;
    char line[256];
    if (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
            if (sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice) == 10) {
                unsigned long long _idle = idle + iowait;
                unsigned long long _non_idle = user + nice + system + irq + softirq + steal;
                unsigned long long _total = _idle + _non_idle;
                
                unsigned long long totald = _total - last_total_cpu;
                unsigned long long idled = _idle - last_idle_cpu;
                
                if (totald > 0) {
                    *usage = (double)(totald - idled) / (double)totald;
                }
                
                last_total_cpu = _total;
                last_idle_cpu = _idle;
            }
        }
    }
    fclose(f);
}

static void parse_proc_mem(double *usage, char **txt) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[256];
    unsigned long long total = 0, avail = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line + 9, "%llu", &total);
        else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%llu", &avail);
            break;
        }
    }
    fclose(f);
    if (total > 0) {
        unsigned long long used = total - avail;
        *usage = (double)used / (double)total;
        *txt = g_strdup_printf("%.1f GB / %.1f GB (%.1f%%)", (double)used / 1048576.0, (double)total / 1048576.0, *usage * 100.0);
    }
}

static void on_kill_proc(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *pid = user_data;
    g_autofree char *cmd = g_strdup_printf("pkexec kill -9 %s", pid);
    g_autoptr(GError) err = NULL;
    subprocess_run_sync(cmd, &err);
}

static gboolean on_refresh_monitor(gpointer data) {
    (void)data;
    if (!page_widget) return G_SOURCE_REMOVE;

    double cpu_pct = 0;
    parse_proc_stat(&cpu_pct);
    gtk_progress_bar_set_fraction(pb_cpu, cpu_pct);
    g_autofree char *cpu_txt = g_strdup_printf("%.1f%%", cpu_pct * 100.0);
    gtk_label_set_text(lbl_cpu, cpu_txt);

    double ram_pct = 0;
    char *ram_txt = NULL;
    parse_proc_mem(&ram_pct, &ram_txt);
    gtk_progress_bar_set_fraction(pb_ram, ram_pct);
    if (ram_txt) {
        gtk_label_set_text(lbl_ram, ram_txt);
        g_free(ram_txt);
    }

    // Refresh Processes
    if (proc_list_box) {
        gtk_list_box_remove_all(GTK_LIST_BOX(proc_list_box));

        g_autoptr(GError) err = NULL;
        // pid, pcpu, pmem, user, comm
        g_autofree char *out = subprocess_run_sync("ps -eo pid,pcpu,pmem,user,comm --sort=-%cpu | head -n 11", &err);
        if (out) {
            g_auto(GStrv) lines = g_strsplit(out, "\n", -1);
            for (int i = 1; lines[i] != NULL; i++) {
                char *line = g_strstrip(g_strdup(lines[i]));
                if (strlen(line) == 0) { g_free(line); continue; }

                // Very primitive splitting by spaces since `comm` doesn't have spaces usually in this format
                g_auto(GStrv) toks = g_strsplit_set(line, " \t", -1);
                GPtrArray *real_toks = g_ptr_array_new();
                for (int j = 0; toks[j] != NULL; j++) {
                    if (strlen(toks[j]) > 0) g_ptr_array_add(real_toks, toks[j]);
                }

                if (real_toks->len >= 5) {
                    const char *pid = g_ptr_array_index(real_toks, 0);
                    const char *cpu = g_ptr_array_index(real_toks, 1);
                    const char *mem = g_ptr_array_index(real_toks, 2);
                    const char *usr = g_ptr_array_index(real_toks, 3);
                    
                    GString *cmdStr = g_string_new("");
                    for (guint j = 4; j < real_toks->len; j++) {
                        g_string_append_printf(cmdStr, "%s ", (const char*)g_ptr_array_index(real_toks, j));
                    }

                    AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
                    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), cmdStr->str);
                    
                    g_autofree char *sub = g_strdup_printf("PID %s  ·  User %s  ·  CPU %s%%  ·  RAM %s%%", pid, usr, cpu, mem);
                    adw_action_row_set_subtitle(row, sub);

                    GtkWidget *kbtn = gtk_button_new_from_icon_name("process-stop-symbolic");
                    gtk_widget_add_css_class(kbtn, "destructive-action");
                    gtk_widget_set_valign(kbtn, GTK_ALIGN_CENTER);
                    g_signal_connect_data(kbtn, "clicked", G_CALLBACK(on_kill_proc), g_strdup(pid), (GClosureNotify)free_g_free, 0);
                    adw_action_row_add_suffix(row, kbtn);

                    gtk_list_box_append(GTK_LIST_BOX(proc_list_box), GTK_WIDGET(row));
                    g_string_free(cmdStr, TRUE);
                }
                g_ptr_array_unref(real_toks);
                g_free(line);
            }
        }
    }

    return G_SOURCE_CONTINUE;
}

static void on_page_destroy(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (timer_id > 0) {
        g_source_remove(timer_id);
        timer_id = 0;
    }
    page_widget = NULL;
}

GtkWidget *page_monitor_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    g_signal_connect(overlay, "destroy", G_CALLBACK(on_page_destroy), NULL);

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "System Monitor");
    adw_preferences_page_set_icon_name(page, "utilities-system-monitor-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    /* ── Resources ─────────────────────────────────────────────────────────── */
    AdwPreferencesGroup *res_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(res_group, "Current Usage");
    adw_preferences_page_add(page, res_group);

    AdwActionRow *row_cpu = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row_cpu), "Processor (CPU)");
    GtkWidget *cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_valign(cpu_box, GTK_ALIGN_CENTER);
    pb_cpu = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_widget_set_size_request(GTK_WIDGET(pb_cpu), 200, -1);
    lbl_cpu = GTK_LABEL(gtk_label_new("0%"));
    gtk_widget_set_halign(GTK_WIDGET(lbl_cpu), GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(cpu_box), GTK_WIDGET(pb_cpu));
    gtk_box_append(GTK_BOX(cpu_box), GTK_WIDGET(lbl_cpu));
    adw_action_row_add_suffix(row_cpu, cpu_box);
    adw_preferences_group_add(res_group, GTK_WIDGET(row_cpu));

    AdwActionRow *row_ram = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row_ram), "Memory (RAM)");
    GtkWidget *ram_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_valign(ram_box, GTK_ALIGN_CENTER);
    pb_ram = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_widget_set_size_request(GTK_WIDGET(pb_ram), 200, -1);
    lbl_ram = GTK_LABEL(gtk_label_new("0 MB"));
    gtk_widget_set_halign(GTK_WIDGET(lbl_ram), GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(ram_box), GTK_WIDGET(pb_ram));
    gtk_box_append(GTK_BOX(ram_box), GTK_WIDGET(lbl_ram));
    adw_action_row_add_suffix(row_ram, ram_box);
    adw_preferences_group_add(res_group, GTK_WIDGET(row_ram));

    /* ── Processes ─────────────────────────────────────────────────────────── */
    proc_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(proc_group, "Top Applications");
    adw_preferences_page_add(page, proc_group);

    proc_list_box = gtk_list_box_new();
    gtk_widget_add_css_class(proc_list_box, "boxed-list");
    adw_preferences_group_add(proc_group, proc_list_box);

    // Initial parse to set last_cpu values
    double tmp;
    parse_proc_stat(&tmp);

    on_refresh_monitor(NULL);
    timer_id = g_timeout_add(2000, on_refresh_monitor, NULL);

    return overlay;
}
