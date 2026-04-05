#include "page-general.h"
#include "../utils/subprocess.h"
#include <adwaita.h>
#include <gio/gio.h>
#include <string.h>

/* ── General page ─────────────────────────────────────────────────────────
 * Shows hostname (editable), OS info, kernel, RAM, GPU, uptime.
 * ─────────────────────────────────────────────────────────────────────────*/

static void on_hostname_apply(AdwEntryRow *row, gpointer user_data) {
    (void)user_data;
    const char *new_hostname = gtk_editable_get_text(GTK_EDITABLE(row));
    if (strlen(new_hostname) == 0) return;
    g_autofree char *cmd = g_strdup_printf("pkexec hostnamectl set-hostname '%s'", new_hostname);
    subprocess_run_async(cmd, NULL, NULL);
}

static char *get_ram_info(void) {
    g_autofree char *contents = NULL;
    if (g_file_get_contents("/proc/meminfo", &contents, NULL, NULL)) {
        g_auto(GStrv) lines = g_strsplit(contents, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            if (g_str_has_prefix(lines[i], "MemTotal:")) {
                long kb = 0;
                sscanf(lines[i], "MemTotal: %ld kB", &kb);
                return g_strdup_printf("%.1f GB", kb / 1024.0 / 1024.0);
            }
        }
    }
    return g_strdup("Unknown");
}

static char *get_gpu_info(void) {
    g_autoptr(GError) err = NULL;
    g_autofree char *lspci = subprocess_run_sync("lspci", &err);
    if (lspci) {
        g_auto(GStrv) lines = g_strsplit(lspci, "\n", -1);
        for (int i = 0; lines[i] != NULL; i++) {
            if (strstr(lines[i], "VGA compatible controller") || strstr(lines[i], "3D controller")) {
                char *gpu = strstr(lines[i], ": ");
                if (gpu) {
                    char *res = g_strdup(gpu + 2);
                    /* remove trailing revision info if any like " (rev 0c)" */
                    char *rev = strrchr(res, '(');
                    if (rev) *rev = '\0';
                    return g_strstrip(res);
                }
            }
        }
    }
    return g_strdup("Unknown");
}

GtkWidget *page_general_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "General");
    adw_preferences_page_set_icon_name(page, "preferences-system-symbolic");

    /* ── About This System ───────────────────────────────────────────── */
    AdwPreferencesGroup *sys_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(sys_group, "System Information");
    adw_preferences_page_add(page, sys_group);

    /* OS name */
    g_autoptr(GError) err = NULL;
    g_autofree char *os_out = subprocess_run_sync("bash -c \". /etc/os-release && echo $PRETTY_NAME\"", &err);
    if (os_out) g_strstrip(os_out);
    AdwActionRow *os_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(os_row), "Operating System");
    adw_action_row_set_subtitle(os_row, (os_out && strlen(os_out) > 0) ? os_out : "Arch Linux");
    adw_preferences_group_add(sys_group, GTK_WIDGET(os_row));

    /* Kernel */
    g_autoptr(GError) err2 = NULL;
    g_autofree char *kernel = subprocess_run_sync("uname -r", &err2);
    if (kernel) g_strstrip(kernel);
    AdwActionRow *kernel_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(kernel_row), "Kernel");
    adw_action_row_set_subtitle(kernel_row, kernel ? kernel : "Unknown");
    adw_preferences_group_add(sys_group, GTK_WIDGET(kernel_row));

    /* RAM */
    g_autofree char *ram = get_ram_info();
    AdwActionRow *ram_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ram_row), "Memory");
    adw_action_row_set_subtitle(ram_row, ram);
    adw_preferences_group_add(sys_group, GTK_WIDGET(ram_row));

    /* GPU */
    g_autofree char *gpu = get_gpu_info();
    AdwActionRow *gpu_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gpu_row), "Graphics");
    adw_action_row_set_subtitle(gpu_row, gpu);
    adw_preferences_group_add(sys_group, GTK_WIDGET(gpu_row));

    /* Uptime */
    g_autoptr(GError) err3 = NULL;
    g_autofree char *uptime = subprocess_run_sync("uptime -p", &err3);
    if (uptime) g_strstrip(uptime);
    AdwActionRow *uptime_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(uptime_row), "Uptime");
    adw_action_row_set_subtitle(uptime_row, uptime ? uptime : "Unknown");
    adw_preferences_group_add(sys_group, GTK_WIDGET(uptime_row));

    /* ── Hostname (editable) ─────────────────────────────────────────── */
    AdwPreferencesGroup *host_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(host_group, "Network Identity");
    adw_preferences_group_set_description(host_group,
        "Press Enter after editing to apply the new hostname. You may be prompted for password.");
    adw_preferences_page_add(page, host_group);

    g_autoptr(GError) err4 = NULL;
    g_autofree char *hostname = subprocess_run_sync("hostname", &err4);
    if (hostname) g_strstrip(hostname);

    AdwEntryRow *host_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(host_row), "Hostname");
    if (hostname) gtk_editable_set_text(GTK_EDITABLE(host_row), hostname);
    g_signal_connect(host_row, "apply", G_CALLBACK(on_hostname_apply), NULL);
    adw_preferences_group_add(host_group, GTK_WIDGET(host_row));

    return GTK_WIDGET(page);
}
