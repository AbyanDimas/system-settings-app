#include "window.h"
#include <gtk/gtk.h>

#include "pages/page-network.h"
#include "pages/page-hotspot.h"
#include "pages/page-bluetooth.h"
#include "pages/page-vpn.h"
#include "pages/page-hosts.h"
#include "pages/page-netmonitor.h"
#include "pages/page-monitor.h"

#include "pages/page-notifications.h"
#include "pages/page-focus.h"
#include "pages/page-screentime.h"

#include "pages/page-general.h"
#include "pages/page-sysinfo.h"
#include "pages/page-displays.h"
#include "pages/page-sound.h"
#include "pages/page-battery.h"
#include "pages/page-privacy.h"
#include "pages/page-update.h"
#include "pages/page-snapshots.h"
#include "pages/page-storage.h"
#include "pages/page-hardware.h"
#include "pages/page-services.h"
#include "pages/page-logs.h"

#include "pages/page-appearance.h"
#include "pages/page-waybar.h"
#include "pages/page-wallpaper.h"
#include "pages/page-looknfeel.h"
#include "pages/page-shaders.h"
#include "pages/page-touchpad.h"
#include "pages/page-startup.h"
#include "pages/page-keybinds.h"
#include "pages/page-input.h"
#include "pages/page-hypridle.h"
#include "pages/page-hyprlock.h"

#include "pages/page-terminal.h"
#include "pages/page-walker.h"
#include "pages/page-shell.h"
#include "pages/page-theme.h"
#include "pages/page-fonts.h"
#include "pages/page-xcompose.h"
#include "pages/page-starship.h"
#include "pages/page-filetools.h"

#include "pages/page-fingerprint.h"
#include "pages/page-security.h"
#include "pages/page-firewall.h"
#include "pages/page-intrusion.h"
#include "pages/page-ssh.h"
#include "pages/page-encryption.h"
#include "pages/page-gpg.h"
#include "pages/page-privacy-dashboard.h"
#include "pages/page-app-permissions.h"

#include "pages/page-docker.h"
#include "pages/page-ollama.h"
#include "pages/page-gamemode.h"
#include "pages/page-mise.h"
#include "pages/page-github.h"
#include "pages/page-ai.h"
#include "pages/page-windowsvm.h"

// New Includes
#include "pages/page-users.h"
#include "pages/page-locale.h"
#include "pages/page-defaultapps.h"
#include "pages/page-printers.h"
#include "pages/page-pacman.h"
#include "pages/page-accessibility.h"
#include "pages/page-boot.h"

static void on_copy_install_cmd(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *cmd = (const char *)user_data;
    GdkClipboard *cb = gdk_display_get_clipboard(gdk_display_get_default());
    gdk_clipboard_set_text(cb, cmd);
}

GtkWidget *omarchy_make_unavailable_page(
  const char *tool_name,
  const char *install_cmd,
  const char *icon_name
) {
  AdwStatusPage *page = ADW_STATUS_PAGE(adw_status_page_new());
  adw_status_page_set_icon_name(page, icon_name ? icon_name : "dialog-warning-symbolic");
  adw_status_page_set_title(page, "Not Available");
  
  char *desc = g_strdup_printf(
    "%s was not found on your system.\n\nInstall it with:\n<tt>%s</tt>",
    tool_name, install_cmd
  );
  adw_status_page_set_description(page, desc);
  g_free(desc);
  
  if (install_cmd && strlen(install_cmd) > 0) {
      GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label("Copy Install Command"));
      gtk_widget_set_halign(GTK_WIDGET(btn), GTK_ALIGN_CENTER);
      gtk_widget_add_css_class(GTK_WIDGET(btn), "pill");
      g_signal_connect_data(btn, "clicked",
        G_CALLBACK(on_copy_install_cmd),
        g_strdup(install_cmd), (GClosureNotify)g_free, 0);
      adw_status_page_set_child(page, GTK_WIDGET(btn));
  }
  
  return GTK_WIDGET(page);
}

struct _OmarchySettingsWindow {
    AdwApplicationWindow parent_instance;
    AdwNavigationSplitView *split_view;
    AdwNavigationView *nav_view;
    AdwToastOverlay *toast_overlay;
    GtkWidget *search_entry;
    GList *sidebar_rows;
    GList *sidebar_groups;
};

G_DEFINE_TYPE(OmarchySettingsWindow, omarchy_settings_window, ADW_TYPE_APPLICATION_WINDOW)

typedef struct {
    AdwActionRow *row;
    AdwPreferencesGroup *group;
    char *search_text;
    gboolean is_main;
} SidebarRowData;

static void on_search_changed(GtkSearchEntry *entry, OmarchySettingsWindow *self) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    g_autofree char *lower_text = g_utf8_strdown(text, -1);
    gboolean is_searching = (strlen(lower_text) > 0);

    // Filter rows
    for (GList *l = self->sidebar_rows; l != NULL; l = l->next) {
        SidebarRowData *data = l->data;
        gboolean visible;
        if (!is_searching) {
            visible = data->is_main;
        } else {
            // Main page row like "Advanced" or "System Info" should also match if searched!
            // But if it's "Advanced & Lainnya" maybe we hide it during search if it doesn't match? Yes
            visible = (strstr(data->search_text, lower_text) != NULL);
        }
        gtk_widget_set_visible(GTK_WIDGET(data->row), visible);
    }

    // Hide empty groups
    for (GList *lg = self->sidebar_groups; lg != NULL; lg = lg->next) {
        AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(lg->data);
        gboolean any_visible = FALSE;

        for (GList *lr = self->sidebar_rows; lr != NULL; lr = lr->next) {
            SidebarRowData *rdata = lr->data;
            if (rdata->group == group && gtk_widget_get_visible(GTK_WIDGET(rdata->row))) {
                any_visible = TRUE;
                break;
            }
        }
        gtk_widget_set_visible(GTK_WIDGET(group), any_visible);
    }
}

typedef struct {
    const char *title;
    const char *icon;
    GtkWidget *(*create_page_func)(void);
} PageInfo;

static const PageInfo conn_pages[] = {
    {"Network", "network-wireless-symbolic", page_network_new},
    {"Bluetooth", "bluetooth-active-symbolic", page_bluetooth_new},
};

static const PageInfo pers_pages[] = {
    {"Appearance", "preferences-desktop-appearance-symbolic", page_appearance_new},
    {"Wallpaper", "preferences-desktop-wallpaper-symbolic", page_wallpaper_new},
    {"Displays", "video-display-symbolic", page_displays_new},
    {"Sound", "audio-volume-high-symbolic", page_sound_new},
};

static const PageInfo dev_pages[] = {
    {"Power and Battery", "battery-good-symbolic", page_battery_new},
    {"Touchpad and Mouse", "input-mouse-symbolic", page_touchpad_new},
    {"Printers and Scanners", "printer-symbolic", page_printers_new},
};

static const PageInfo sys_pages[] = {
    {"Users and Accounts", "system-users-symbolic", page_users_new},
    {"Time and Language", "preferences-desktop-locale-symbolic", page_locale_new},
    {"Default Apps", "applications-system-symbolic", page_defaultapps_new},
    {"Software Update", "software-update-available-symbolic", page_update_new},
};

// Declared ahead
static GtkWidget *page_others_new(void);

static const PageInfo adv_main_pages[] = {
    {"Advanced and Others", "applications-other-symbolic", page_others_new},
};

typedef struct {
    const char *category;
    const PageInfo *pages;
    size_t num_pages;
} CategoryInfo;

static const CategoryInfo categories[] = {
    {"Connectivity", conn_pages, G_N_ELEMENTS(conn_pages)},
    {"Personalization", pers_pages, G_N_ELEMENTS(pers_pages)},
    {"Device", dev_pages, G_N_ELEMENTS(dev_pages)},
    {"System", sys_pages, G_N_ELEMENTS(sys_pages)},
    {"Advanced", adv_main_pages, G_N_ELEMENTS(adv_main_pages)},
};

/* --- OTHERS (DETAILED) PAGES --- */

static const PageInfo other_network[] = {
    {"Hotspot", "network-wireless-hotspot-symbolic", page_hotspot_new},
    {"VPN", "network-vpn-symbolic", page_vpn_new},
    {"Hosts File", "network-wired-symbolic", page_hosts_new},
    {"Network Monitor", "network-transmit-receive-symbolic", page_netmonitor_new},
    {"System Monitor", "utilities-system-monitor-symbolic", page_monitor_new},
};

static const PageInfo other_desktop[] = {
    {"Look and Feel", "preferences-desktop-theme-symbolic", page_looknfeel_new},
    {"Theme Colors", "preferences-desktop-wallpaper-symbolic", page_theme_new},
    {"Fonts", "preferences-desktop-font-symbolic", page_fonts_new},
    {"Shaders", "preferences-desktop-display-symbolic", page_shaders_new},
    {"Waybar Config", "applications-system-symbolic", page_waybar_new},
    {"Idle and Sleep", "system-suspend-symbolic", page_hypridle_new},
    {"Lock Screen", "changes-prevent-symbolic", page_hyprlock_new},
    {"Accessibility", "preferences-desktop-accessibility-symbolic", page_accessibility_new},
    {"Notifications", "preferences-system-notifications-symbolic", page_notifications_new},
};

static const PageInfo other_system[] = {
    {"General Info", "preferences-system-symbolic", page_general_new},
    {"System Info (Neofetch)", "emblem-system-symbolic", page_sysinfo_new},
    {"Hardware", "device-notifier-symbolic", page_hardware_new},
    {"Snapshots (BTRFS)", "drive-harddisk-symbolic", page_snapshots_new},
    {"Disk Storage", "drive-harddisk-system-symbolic", page_storage_new},
    {"Services (Systemd)", "applications-system-symbolic", page_services_new},
    {"System Logs", "format-text-strikethrough-symbolic", page_logs_new},
    {"Pacman (Mirror Area)", "system-software-install-symbolic", page_pacman_new},
    {"Boot and Kernel", "drive-harddisk-system-symbolic", page_boot_new},
};

static const PageInfo other_input[] = {
    {"Input Devices", "input-keyboard-symbolic", page_input_new},
    {"Keybinds (Shortcuts)", "preferences-desktop-keyboard-shortcuts-symbolic", page_keybinds_new},
    {"XCompose", "input-keyboard-symbolic", page_xcompose_new},
};

static const PageInfo other_apps[] = {
    {"Terminal Emulator", "utilities-terminal-symbolic", page_terminal_new},
    {"Startup Apps", "system-run-symbolic", page_startup_new},
    {"App Launcher Menu", "system-search-symbolic", page_walker_new},
    {"Shell Config", "utilities-terminal-symbolic", page_shell_new},
    {"Starship Prompt", "utilities-terminal-symbolic", page_starship_new},
    {"File Utilities", "system-search-symbolic", page_filetools_new},
    {"Docker", "applications-other-symbolic", page_docker_new},
    {"Ollama (AI)", "applications-science-symbolic", page_ollama_new},
    {"Game Mode", "input-gaming-symbolic", page_gamemode_new},
    {"Mise", "applications-system-symbolic", page_mise_new},
    {"GitHub", "applications-other-symbolic", page_github_new},
    {"AI Integration", "applications-science-symbolic", page_ai_new},
    {"Windows VM", "applications-other-symbolic", page_windowsvm_new},
};

static const PageInfo other_security[] = {
    {"Security Overview", "security-high-symbolic", page_security_new},
    {"Fingerprint", "security-high-symbolic", page_fingerprint_new},
    {"Firewall", "network-firewall-symbolic", page_firewall_new},
    {"Intrusion Detection", "security-medium-symbolic", page_intrusion_new},
    {"SSH Keys", "network-wired-symbolic", page_ssh_new},
    {"Disk Encryption", "drive-harddisk-system-symbolic", page_encryption_new},
    {"GPG", "emblem-readonly-symbolic", page_gpg_new},
    {"Privacy Dashboard", "security-high-symbolic", page_privacy_dashboard_new},
    {"App Permissions", "security-low-symbolic", page_app_permissions_new},
    {"Focus Mode", "applications-utilities-symbolic", page_focus_new},
    {"Screen Time", "preferences-system-time-symbolic", page_screentime_new},
};

static const CategoryInfo other_categories[] = {
    {"Network and Connectivity", other_network, G_N_ELEMENTS(other_network)},
    {"Desktop and Layout", other_desktop, G_N_ELEMENTS(other_desktop)},
    {"Hardware and System", other_system, G_N_ELEMENTS(other_system)},
    {"Input and Keyboards", other_input, G_N_ELEMENTS(other_input)},
    {"Applications and Developer", other_apps, G_N_ELEMENTS(other_apps)},
    {"Security and Privacy", other_security, G_N_ELEMENTS(other_security)},
};

static void on_other_row_activated(AdwActionRow *row, gpointer user_data) {
    (void)user_data;
    const char *title = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    GtkWidget *page_widget = NULL;
    for (size_t c = 0; c < G_N_ELEMENTS(other_categories); c++) {
        for (size_t p = 0; p < other_categories[c].num_pages; p++) {
            if (g_strcmp0(other_categories[c].pages[p].title, title) == 0) {
                page_widget = other_categories[c].pages[p].create_page_func();
                break;
            }
        }
        if (page_widget) break;
    }
    
    if (page_widget) {
        AdwNavigationView *nav_view = ADW_NAVIGATION_VIEW(gtk_widget_get_ancestor(GTK_WIDGET(row), ADW_TYPE_NAVIGATION_VIEW));
        if (nav_view) {
            AdwNavigationPage *nav_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(page_widget, title));
            adw_navigation_view_push(nav_view, nav_page);
        }
    }
}

static GtkWidget *page_others_new(void) {
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_page_set_title(page, "Advanced Settings");
    adw_preferences_page_set_icon_name(page, "applications-system-symbolic");

    for (size_t c = 0; c < G_N_ELEMENTS(other_categories); c++) {
        AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(group, other_categories[c].category);
        
        for (size_t p = 0; p < other_categories[c].num_pages; p++) {
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), other_categories[c].pages[p].title);
            adw_action_row_add_prefix(row, gtk_image_new_from_icon_name(other_categories[c].pages[p].icon));
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("go-next-symbolic"));
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
            g_signal_connect(row, "activated", G_CALLBACK(on_other_row_activated), NULL);
            adw_preferences_group_add(group, GTK_WIDGET(row));
        }
        adw_preferences_page_add(page, group);
    }
    return GTK_WIDGET(page);
}

static void on_sidebar_row_activated(AdwActionRow *row, OmarchySettingsWindow *self) {
    const char *title = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    // Check main categories
    for (size_t c = 0; c < G_N_ELEMENTS(categories); c++) {
        for (size_t p = 0; p < categories[c].num_pages; p++) {
            if (g_strcmp0(categories[c].pages[p].title, title) == 0) {
                GtkWidget *page_widget = categories[c].pages[p].create_page_func();
                AdwNavigationPage *nav_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(page_widget, title));
                adw_navigation_view_push(self->nav_view, nav_page);
                adw_navigation_split_view_set_show_content(self->split_view, TRUE);
                return;
            }
        }
    }
    // Check other categories (for Global Search!)
    for (size_t c = 0; c < G_N_ELEMENTS(other_categories); c++) {
        for (size_t p = 0; p < other_categories[c].num_pages; p++) {
            if (g_strcmp0(other_categories[c].pages[p].title, title) == 0) {
                GtkWidget *page_widget = other_categories[c].pages[p].create_page_func();
                AdwNavigationPage *nav_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(page_widget, title));
                adw_navigation_view_push(self->nav_view, nav_page);
                adw_navigation_split_view_set_show_content(self->split_view, TRUE);
                return;
            }
        }
    }
}

static void setup_ui(OmarchySettingsWindow *self) {
    self->toast_overlay = ADW_TOAST_OVERLAY(adw_toast_overlay_new());
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), GTK_WIDGET(self->toast_overlay));

    self->split_view = ADW_NAVIGATION_SPLIT_VIEW(adw_navigation_split_view_new());
    adw_toast_overlay_set_child(self->toast_overlay, GTK_WIDGET(self->split_view));

    // Content view
    self->nav_view = ADW_NAVIGATION_VIEW(adw_navigation_view_new());
    
    // Default home page in content
    AdwStatusPage *home_page = ADW_STATUS_PAGE(adw_status_page_new());
    adw_status_page_set_title(home_page, "System Overview");
    adw_status_page_set_icon_name(home_page, "computer-symbolic");
    adw_status_page_set_description(home_page, "Select a category from the sidebar to view its settings.");
    
    AdwNavigationPage *home_nav_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(GTK_WIDGET(home_page), "Home"));
    adw_navigation_view_add(self->nav_view, home_nav_page);

    // The nav_view itself must be inside a page to be set as content of split_view
    AdwNavigationPage *content_container_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(GTK_WIDGET(self->nav_view), "Content"));
    adw_navigation_split_view_set_content(self->split_view, content_container_page);

    // Sidebar with ToolbarView to hold the search entry in the header
    AdwToolbarView *sidebar_toolbar = ADW_TOOLBAR_VIEW(adw_toolbar_view_new());
    AdwHeaderBar *sidebar_header = ADW_HEADER_BAR(adw_header_bar_new());
    adw_header_bar_set_show_title(sidebar_header, FALSE);
    
    self->search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(self->search_entry, TRUE);
    adw_header_bar_set_title_widget(sidebar_header, self->search_entry);
    g_signal_connect(self->search_entry, "search-changed", G_CALLBACK(on_search_changed), self);
    
    adw_toolbar_view_add_top_bar(sidebar_toolbar, GTK_WIDGET(sidebar_header));

    AdwPreferencesPage *sidebar_page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    
    // Dedicated box for sidebar content
    GtkWidget *sidebar_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_toolbar_view_set_content(sidebar_toolbar, sidebar_vbox);

    // Search bar at the top, outside of scrolled area if possible, or just first child
    self->search_entry = gtk_search_entry_new();
    gtk_widget_set_margin_start(self->search_entry, 12);
    gtk_widget_set_margin_end(self->search_entry, 12);
    gtk_widget_set_margin_top(self->search_entry, 12);
    gtk_widget_set_margin_bottom(self->search_entry, 6);
    g_signal_connect(self->search_entry, "search-changed", G_CALLBACK(on_search_changed), self);
    gtk_box_append(GTK_BOX(sidebar_vbox), self->search_entry);

    //Scrolled window for the preferences page
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), GTK_WIDGET(sidebar_page));
    gtk_box_append(GTK_BOX(sidebar_vbox), scrolled);

    self->sidebar_rows = NULL;
    self->sidebar_groups = NULL;

    for (size_t c = 0; c < G_N_ELEMENTS(categories); c++) {
        AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
        adw_preferences_group_set_title(group, categories[c].category);
        self->sidebar_groups = g_list_append(self->sidebar_groups, group);

        for (size_t p = 0; p < categories[c].num_pages; p++) {
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), categories[c].pages[p].title);
            adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
            adw_action_row_add_prefix(row, gtk_image_new_from_icon_name(categories[c].pages[p].icon));
            adw_action_row_add_suffix(row, gtk_image_new_from_icon_name("go-next-symbolic"));
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
            g_signal_connect(row, "activated", G_CALLBACK(on_sidebar_row_activated), self);
            adw_preferences_group_add(group, GTK_WIDGET(row));

            SidebarRowData *data = g_new0(SidebarRowData, 1);
            data->row = row;
            data->group = group;
            data->search_text = g_utf8_strdown(categories[c].pages[p].title, -1);
            data->is_main = TRUE;
            self->sidebar_rows = g_list_append(self->sidebar_rows, data);
        }
        adw_preferences_page_add(sidebar_page, group);
    }

    // Embed all other pages in a "Search Results" group that is normally hidden!
    AdwPreferencesGroup *search_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(search_group, "Deep Settings");
    gtk_widget_set_visible(GTK_WIDGET(search_group), FALSE);
    self->sidebar_groups = g_list_append(self->sidebar_groups, search_group);
    
    for (size_t c = 0; c < G_N_ELEMENTS(other_categories); c++) {
        for (size_t p = 0; p < other_categories[c].num_pages; p++) {
            AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), other_categories[c].pages[p].title);
            adw_action_row_add_prefix(row, gtk_image_new_from_icon_name(other_categories[c].pages[p].icon));
            gtk_widget_set_visible(GTK_WIDGET(row), FALSE);
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
            g_signal_connect(row, "activated", G_CALLBACK(on_sidebar_row_activated), self);
            adw_preferences_group_add(search_group, GTK_WIDGET(row));

            SidebarRowData *data = g_new0(SidebarRowData, 1);
            data->row = row;
            data->group = search_group;
            data->search_text = g_utf8_strdown(other_categories[c].pages[p].title, -1);
            data->is_main = FALSE;
            self->sidebar_rows = g_list_append(self->sidebar_rows, data);
        }
    }
    adw_preferences_page_add(sidebar_page, search_group);

    AdwNavigationPage *sidebar_nav_page = adw_navigation_page_new(GTK_WIDGET(sidebar_toolbar), "Settings");
    adw_navigation_page_set_title(sidebar_nav_page, "Settings");
    adw_navigation_split_view_set_sidebar(self->split_view, sidebar_nav_page);
    adw_navigation_split_view_set_sidebar_width_fraction(self->split_view, 0.28);
}

static void omarchy_settings_window_class_init(OmarchySettingsWindowClass *klass) {
    (void)klass;
}

static void omarchy_settings_window_init(OmarchySettingsWindow *self) {
    gtk_window_set_default_size(GTK_WINDOW(self), 1100, 750);
    setup_ui(self);
}

OmarchySettingsWindow *omarchy_settings_window_new(OmarchySettingsApp *app) {
    return g_object_new(OMARCHY_TYPE_SETTINGS_WINDOW, "application", app, NULL);
}
