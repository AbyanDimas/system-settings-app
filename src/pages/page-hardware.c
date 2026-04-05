#include "page-hardware.h"
#include "../utils/subprocess.h"
#include "../utils/tool-detector.h"
#include "../window.h"
#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

static GtkWidget *page_widget = NULL;

static char *read_one_line(const char *path) {
    g_autofree char *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        char *str = g_strstrip(g_strdup(contents));
        return str;
    }
    return NULL;
}

GtkWidget *page_hardware_new(void) {
    GtkWidget *overlay = adw_toast_overlay_new();
    page_widget = overlay;

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Hardware Info");
    adw_preferences_page_set_icon_name(page, "processor-symbolic");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(overlay), GTK_WIDGET(page));

    g_autoptr(GError) err = NULL;

    /* ── Core System ───────────────────────────────────────────────────────── */
    AdwPreferencesGroup *sys_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(sys_group, "Specifications");
    adw_preferences_page_add(page, sys_group);

    // CPU
    AdwActionRow *r_cpu = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_cpu), "Processor");
    g_autofree char *cpu_out = subprocess_run_sync("lscpu | grep 'Model name:' | sed 's/Model name:[ \t]*//'", &err);
    adw_action_row_set_subtitle(r_cpu, cpu_out && strlen(cpu_out) > 0 ? g_strstrip(cpu_out) : "Unknown Intel/AMD CPU");
    adw_preferences_group_add(sys_group, GTK_WIDGET(r_cpu));

    // GPU
    AdwActionRow *r_gpu = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_gpu), "Graphics (GPU)");
    g_autofree char *gpu_out = subprocess_run_sync("lspci | grep -i 'vga\\|3d\\|display' | head -n 1 | cut -d ':' -f 3 | sed 's/^[ \t]*//'", &err);
    adw_action_row_set_subtitle(r_gpu, gpu_out && strlen(gpu_out) > 0 ? g_strstrip(gpu_out) : "Unknown Graphics");
    adw_preferences_group_add(sys_group, GTK_WIDGET(r_gpu));

    // RAM
    AdwActionRow *r_ram = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_ram), "Memory (RAM)");
    g_autofree char *ram_out = subprocess_run_sync("grep MemTotal /proc/meminfo | awk '{print $2}'", &err);
    if (ram_out && strlen(ram_out) > 0) {
        double gb = atof(ram_out) / 1048576.0;
        g_autofree char *rs = g_strdup_printf("%.1f GB", gb);
        adw_action_row_set_subtitle(r_ram, rs);
    }
    adw_preferences_group_add(sys_group, GTK_WIDGET(r_ram));

    // Motherboard
    AdwActionRow *r_mobo = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_mobo), "Motherboard");
    g_autofree char *vendor = read_one_line("/sys/class/dmi/id/board_vendor");
    g_autofree char *board = read_one_line("/sys/class/dmi/id/board_name");
    if (vendor && board) {
        g_autofree char *mobo = g_strdup_printf("%s %s", vendor, board);
        adw_action_row_set_subtitle(r_mobo, mobo);
    } else {
        adw_action_row_set_subtitle(r_mobo, "Virtual Machine or Unknown");
    }
    adw_preferences_group_add(sys_group, GTK_WIDGET(r_mobo));

    // OS / Kernel
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        AdwActionRow *r_kern = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_kern), "Linux Kernel");
        g_autofree char *kv = g_strdup_printf("%s %s", buffer.sysname, buffer.release);
        adw_action_row_set_subtitle(r_kern, kv);
        adw_preferences_group_add(sys_group, GTK_WIDGET(r_kern));
    }

    /* ── Sensors / Temperatures ────────────────────────────────────────────── */
    if (g_find_program_in_path("sensors")) {
        AdwPreferencesGroup *sens_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(sens_group, "Temperatures");
        adw_preferences_page_add(page, sens_group);

        g_autofree char *s_out = subprocess_run_sync("sensors", &err);
        if (s_out && strlen(s_out) > 0) {
            AdwExpanderRow *exp = ADW_EXPANDER_ROW(adw_expander_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(exp), "LM Sensors Overview");
            adw_expander_row_set_subtitle(exp, "Thermal zones across your system");
            
            g_auto(GStrv) lines = g_strsplit(s_out, "\n", -1);
            int count = 0;
            for (int i = 0; lines[i] != NULL; i++) {
                char *l = g_strstrip(g_strdup(lines[i]));
                if (strlen(l) > 0 && strstr(l, "°C")) {
                    AdwActionRow *sr = ADW_ACTION_ROW(adw_action_row_new());
                    
                    g_auto(GStrv) toks = g_strsplit(l, ":", 2);
                    if (toks[0] && toks[1]) {
                        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr), g_strstrip(g_strdup(toks[0])));
                        adw_action_row_set_subtitle(sr, g_strstrip(g_strdup(toks[1])));
                    } else {
                        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(sr), l);
                    }
                    adw_expander_row_add_row(exp, GTK_WIDGET(sr));
                    count++;
                }
                g_free(l);
            }
            if (count > 0) adw_preferences_group_add(sens_group, GTK_WIDGET(exp));
        }
    }

    /* ── Battery Health ────────────────────────────────────────────────────── */
    const char *bats[] = {"/sys/class/power_supply/BAT0", "/sys/class/power_supply/BAT1"};
    for (int i = 0; i < 2; i++) {
        if (g_file_test(bats[i], G_FILE_TEST_EXISTS)) {
            AdwPreferencesGroup *bat_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
            adw_preferences_group_set_title(bat_group, g_strdup_printf("Battery (%d)", i));
            adw_preferences_page_add(page, bat_group);

            g_autofree char *p_cap = g_build_filename(bats[i], "capacity", NULL);
            g_autofree char *p_enow = g_build_filename(bats[i], "energy_now", NULL);
            g_autofree char *p_efull = g_build_filename(bats[i], "energy_full", NULL);
            g_autofree char *p_cyc = g_build_filename(bats[i], "cycle_count", NULL);
            g_autofree char *p_stat = g_build_filename(bats[i], "status", NULL);
            g_autofree char *p_man = g_build_filename(bats[i], "manufacturer", NULL);

            char *cap_s = read_one_line(p_cap);
            char *stat_s = read_one_line(p_stat);
            char *cyc_s = read_one_line(p_cyc);
            char *man_s = read_one_line(p_man);
            
            AdwActionRow *r_b1 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_b1), "Current Charge");
            adw_action_row_set_subtitle(r_b1, cap_s ? g_strdup_printf("%s%% (%s)", cap_s, stat_s ? stat_s : "") : "Unknown");
            adw_preferences_group_add(bat_group, GTK_WIDGET(r_b1));

            AdwActionRow *r_b2 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_b2), "Cycle Count");
            adw_action_row_set_subtitle(r_b2, cyc_s ? cyc_s : "Unknown");
            adw_preferences_group_add(bat_group, GTK_WIDGET(r_b2));

            AdwActionRow *r_b3 = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(r_b3), "Manufacturer");
            adw_action_row_set_subtitle(r_b3, man_s ? man_s : "Unknown");
            adw_preferences_group_add(bat_group, GTK_WIDGET(r_b3));

            g_free(cap_s); g_free(stat_s); g_free(cyc_s); g_free(man_s);
        }
    }

    return overlay;
}
