#ifndef TOOL_DETECTOR_H
#define TOOL_DETECTOR_H

#include <glib.h>

typedef struct {
    gboolean has_nmcli;
    gboolean has_iwd;
    gboolean has_wpa_cli;
    gboolean has_bluetoothctl;
    gboolean has_rfkill;
    gboolean has_pactl;
    gboolean has_wpctl;
    gboolean has_pw_cli;
    gboolean has_hyprctl;
    gboolean has_swaymsg;
    gboolean has_wlr_randr;
    gboolean has_kanshi;
    gboolean has_swww;
    gboolean has_swaybg;
    gboolean has_feh;
    gboolean has_mako;
    gboolean has_dunst;
    gboolean has_makoctl;
    gboolean has_dunstctl;
    gboolean has_powerprofilesctl;
    gboolean has_tlp;
    gboolean has_cpupower;
    gboolean has_ufw;
    gboolean has_firewalld;
    gboolean has_nftables;
    gboolean has_systemctl;
    gboolean has_hostnamectl;
    gboolean has_timedatectl;
    gboolean has_localectl;
    gboolean has_pkexec;
    gboolean has_sudo;
    gboolean has_fprintd;
    gboolean has_pipewire;
    gboolean has_pulseaudio;
    gboolean has_waybar;
    gboolean has_eww;
    gboolean has_gtksourceview;
    gboolean has_walker;
    gboolean has_hyprlock;
    gboolean has_hypridle;
    gboolean has_foot;
    gboolean has_kitty;
    gboolean has_alacritty;
    
    // Hardware paths
    gboolean has_backlight;
    char *backlight_path;
    gboolean has_battery;
    char *battery_path;
    
    // Logical backends resolved
    char *compositor;
    char *network_backend;
    char *audio_backend;
    char *notification_daemon;
    char *wallpaper_tool;
    char *display_tool;
    char *firewall_tool;
    char *power_tool;
    char *idle_daemon;
    char *app_launcher;
    char *terminal;
} OmarchySystemCaps;

// Run once at app startup
OmarchySystemCaps *omarchy_detect_system_caps(void);

// Returns cached result
OmarchySystemCaps *omarchy_get_system_caps(void);

#endif // TOOL_DETECTOR_H
