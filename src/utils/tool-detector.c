#include "tool-detector.h"
#include <gio/gio.h>
#include <string.h>

static OmarchySystemCaps *global_caps = NULL;

static gboolean tool_exists(const char *name) {
    char *path = g_find_program_in_path(name);
    if (path) {
        g_free(path);
        return TRUE;
    }
    
    // Check common sbin paths manually if not in PATH
    const char *extra_paths[] = {"/usr/sbin", "/sbin", "/usr/local/sbin"};
    for (size_t i = 0; i < G_N_ELEMENTS(extra_paths); i++) {
        g_autofree char *full = g_build_filename(extra_paths[i], name, NULL);
        if (g_file_test(full, G_FILE_TEST_IS_EXECUTABLE)) return TRUE;
    }

    return FALSE;
}

static gboolean is_daemon_running(const char *name) {
    // Try pgrep first, then systemctl if available
    g_autofree char *cmd = g_strdup_printf("pgrep -f %s", name);
    int exit_status = 0;
    if (g_spawn_command_line_sync(cmd, NULL, NULL, &exit_status, NULL)) {
        if (g_spawn_check_wait_status(exit_status, NULL)) return TRUE;
    }
    
    // Fallback to systemctl check for some common daemons
    g_autofree char *sys_cmd = g_strdup_printf("systemctl is-active --quiet %s", name);
    if (g_spawn_command_line_sync(sys_cmd, NULL, NULL, &exit_status, NULL)) {
        if (g_spawn_check_wait_status(exit_status, NULL)) return TRUE;
    }

    return FALSE;
}

static void detect_battery_path(OmarchySystemCaps *caps) {
    caps->has_battery = FALSE;
    caps->battery_path = NULL;
    
    GDir *dir = g_dir_open("/sys/class/power_supply", 0, NULL);
    if (!dir) return;
    
    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (g_str_has_prefix(name, "BAT")) {
            caps->has_battery = TRUE;
            caps->battery_path = g_build_filename("/sys/class/power_supply", name, NULL);
            break;
        }
    }
    g_dir_close(dir);
}

static void detect_backlight_path(OmarchySystemCaps *caps) {
    caps->has_backlight = FALSE;
    caps->backlight_path = NULL;
    
    GDir *dir = g_dir_open("/sys/class/backlight", 0, NULL);
    if (!dir) return;
    
    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        caps->has_backlight = TRUE;
        caps->backlight_path = g_build_filename("/sys/class/backlight", name, NULL);
        break;
    }
    g_dir_close(dir);
}

OmarchySystemCaps *omarchy_detect_system_caps(void) {
    if (global_caps) return global_caps;

    global_caps = g_new0(OmarchySystemCaps, 1);

    // Tools
    global_caps->has_nmcli = tool_exists("nmcli");
    global_caps->has_iwd = tool_exists("iwd") || tool_exists("iwctl");
    global_caps->has_wpa_cli = tool_exists("wpa_cli");
    global_caps->has_bluetoothctl = tool_exists("bluetoothctl") || tool_exists("bluetoothd") || tool_exists("omarchy-launch-bluetooth");
    global_caps->has_rfkill = tool_exists("rfkill");
    global_caps->has_pactl = tool_exists("pactl");
    global_caps->has_wpctl = tool_exists("wpctl");
    global_caps->has_pw_cli = tool_exists("pw-cli");
    global_caps->has_hyprctl = tool_exists("hyprctl");
    global_caps->has_swaymsg = tool_exists("swaymsg");
    global_caps->has_wlr_randr = tool_exists("wlr-randr");
    global_caps->has_kanshi = tool_exists("kanshi");
    global_caps->has_swww = tool_exists("swww");
    global_caps->has_swaybg = tool_exists("swaybg");
    global_caps->has_feh = tool_exists("feh");
    global_caps->has_mako = tool_exists("mako");
    global_caps->has_dunst = tool_exists("dunst");
    global_caps->has_makoctl = tool_exists("makoctl");
    global_caps->has_dunstctl = tool_exists("dunstctl");
    global_caps->has_powerprofilesctl = tool_exists("powerprofilesctl");
    global_caps->has_tlp = tool_exists("tlp");
    global_caps->has_cpupower = tool_exists("cpupower");
    global_caps->has_ufw = tool_exists("ufw");
    global_caps->has_firewalld = tool_exists("firewalld");
    global_caps->has_nftables = tool_exists("nftables");
    global_caps->has_systemctl = tool_exists("systemctl");
    global_caps->has_hostnamectl = tool_exists("hostnamectl");
    global_caps->has_timedatectl = tool_exists("timedatectl");
    global_caps->has_localectl = tool_exists("localectl");
    global_caps->has_pkexec = tool_exists("pkexec");
    global_caps->has_sudo = tool_exists("sudo");
    global_caps->has_fprintd = tool_exists("fprintd-enroll");
    global_caps->has_waybar = tool_exists("waybar");
    global_caps->has_eww = tool_exists("eww");
    global_caps->has_walker = tool_exists("walker");
    global_caps->has_hyprlock = tool_exists("hyprlock");
    global_caps->has_hypridle = tool_exists("hypridle");
    global_caps->has_foot = tool_exists("foot");
    global_caps->has_kitty = tool_exists("kitty");
    global_caps->has_alacritty = tool_exists("alacritty");

    // Daemons (run status)
    global_caps->has_pipewire = is_daemon_running("pipewire");
    global_caps->has_pulseaudio = is_daemon_running("pulseaudio");

    // Hardware checks
    detect_battery_path(global_caps);
    detect_backlight_path(global_caps);

    /* --- Logical Backends Resolution --- */

    // Compositor
    if (g_getenv("HYPRLAND_INSTANCE_SIGNATURE")) {
        global_caps->compositor = g_strdup("hyprland");
    } else if (g_getenv("SWAYSOCK")) {
        global_caps->compositor = g_strdup("sway");
    } else {
        global_caps->compositor = g_strdup("unknown");
    }

    // Network
    if (is_daemon_running("iwd") || global_caps->has_iwd) {
        global_caps->network_backend = g_strdup("iwd");
    } else if (global_caps->has_nmcli || is_daemon_running("NetworkManager")) {
        global_caps->network_backend = g_strdup("networkmanager");
    } else if (global_caps->has_wpa_cli) {
        global_caps->network_backend = g_strdup("wpa_supplicant");
    } else {
        global_caps->network_backend = g_strdup("iwd"); // Default
    }

    // Audio
    if (global_caps->has_pipewire) {
        global_caps->audio_backend = g_strdup("pipewire");
    } else if (global_caps->has_pulseaudio) {
        global_caps->audio_backend = g_strdup("pulseaudio");
    } else {
        global_caps->audio_backend = g_strdup("none");
    }

    // Notifications
    if (is_daemon_running("mako") || global_caps->has_makoctl) {
        global_caps->notification_daemon = g_strdup("mako");
    } else if (is_daemon_running("dunst") || global_caps->has_dunstctl) {
        global_caps->notification_daemon = g_strdup("dunst");
    } else {
        global_caps->notification_daemon = g_strdup("none");
    }

    // Wallpaper
    if (global_caps->has_swww) {
        global_caps->wallpaper_tool = g_strdup("swww");
    } else if (global_caps->has_swaybg) {
        global_caps->wallpaper_tool = g_strdup("swaybg");
    } else if (global_caps->has_feh) {
        global_caps->wallpaper_tool = g_strdup("feh");
    } else {
        global_caps->wallpaper_tool = g_strdup("none");
    }

    // Display
    if (global_caps->has_hyprctl && strcmp(global_caps->compositor, "hyprland") == 0) {
        global_caps->display_tool = g_strdup("hyprctl");
    } else if (global_caps->has_swaymsg && strcmp(global_caps->compositor, "sway") == 0) {
        global_caps->display_tool = g_strdup("swaymsg");
    } else if (global_caps->has_wlr_randr) {
        global_caps->display_tool = g_strdup("wlr-randr");
    } else {
        global_caps->display_tool = g_strdup("none");
    }

    // Firewall
    if (global_caps->has_ufw) {
        global_caps->firewall_tool = g_strdup("ufw");
    } else if (global_caps->has_firewalld) {
        global_caps->firewall_tool = g_strdup("firewalld");
    } else if (global_caps->has_nftables) {
        global_caps->firewall_tool = g_strdup("nftables");
    } else {
        global_caps->firewall_tool = g_strdup("none");
    }

    // Power
    if (global_caps->has_powerprofilesctl) {
        global_caps->power_tool = g_strdup("powerprofilesctl");
    } else if (global_caps->has_tlp) {
        global_caps->power_tool = g_strdup("tlp");
    } else if (global_caps->has_cpupower) {
        global_caps->power_tool = g_strdup("cpupower");
    } else {
        global_caps->power_tool = g_strdup("none");
    }

    // Idle
    if (global_caps->has_hypridle) {
        global_caps->idle_daemon = g_strdup("hypridle");
    } else if (is_daemon_running("swayidle")) {
        global_caps->idle_daemon = g_strdup("swayidle");
    } else {
        global_caps->idle_daemon = g_strdup("none");
    }

    // Launcher
    if (global_caps->has_walker) {
        global_caps->app_launcher = g_strdup("walker");
    } else if (tool_exists("rofi")) {
        global_caps->app_launcher = g_strdup("rofi");
    } else if (tool_exists("wofi")) {
        global_caps->app_launcher = g_strdup("wofi");
    } else {
        global_caps->app_launcher = g_strdup("none");
    }

    // Terminal
    if (global_caps->has_foot) {
        global_caps->terminal = g_strdup("foot");
    } else if (global_caps->has_kitty) {
        global_caps->terminal = g_strdup("kitty");
    } else if (global_caps->has_alacritty) {
        global_caps->terminal = g_strdup("alacritty");
    } else {
        global_caps->terminal = g_strdup("none");
    }

    return global_caps;
}

OmarchySystemCaps *omarchy_get_system_caps(void) {
    if (!global_caps) {
        return omarchy_detect_system_caps();
    }
    return global_caps;
}
