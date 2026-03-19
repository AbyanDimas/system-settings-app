use iced::{Element, Task, Subscription, Length};
use iced::widget::{row, container, stack};
use std::time::{Duration, Instant};
use chrono::Datelike;

use crate::ui::{homepage, pages, sidebar};
use crate::backend::{
    audio::AudioService, 
    display::DisplayService,
    bluetooth::BluetoothService,
    network::{NetworkService, WifiNetwork, ConnectionDetails},
    power::PowerService,
    hyprland,
    system,
    security,
};
use crate::theme::MacOSTheme;

#[derive(Debug, Clone, PartialEq)]
pub enum Route {
    Home, Display, Network, Hotspot, Bluetooth, Audio, TouchpadMouse, Appearance, Waybar, Keybinds, System, Power, Startup, Security, Notifications, Focus, ScreenTime, Wallpaper, About, SoftwareUpdate, Storage, DateTime, LanguageRegion, GameMode,
}

pub struct App {
    pub route: Route, pub history: Vec<Route>, pub search_query: String, pub theme: MacOSTheme, pub transition_progress: f32,
    pub volume: u32, pub brightness: u32, pub is_muted: bool, pub audio_sinks: Vec<crate::backend::audio::AudioDevice>, pub audio_sources: Vec<crate::backend::audio::AudioDevice>, pub audio_cards: Vec<crate::backend::audio::AudioCard>, 
    pub bluetooth_on: bool, pub bluetooth_auto_on: bool, pub bluetooth_adapter_name: String, pub bluetooth_adapter: Option<crate::backend::bluetooth::BluetoothAdapter>, pub bluetooth_devices: Vec<crate::backend::bluetooth::BluetoothDevice>, pub is_bluetooth_scanning: bool, pub show_bluetooth_rename_modal: bool, pub new_bluetooth_name: String, pub connecting_device_path: Option<String>, pub last_tick: Instant, pub last_data_update: Instant, pub wifi_on: bool, pub is_wifi_scanning: bool, pub current_ssid: String, pub available_networks: Vec<WifiNetwork>, pub dns_servers: Vec<String>, pub vpns: Vec<crate::backend::network::VpnInfo>, pub new_dns_input: String,
    pub show_network_modal: bool, pub target_ssid: String, pub network_password: String, pub network_error: Option<String>, pub show_network_advanced_modal: bool, pub auto_join_networks: std::collections::HashSet<String>, pub connection_details: ConnectionDetails,
    pub hotspot_enabled: bool, pub hotspot_ssid: String, pub hotspot_password: String, pub hotspot_security: String, pub hotspot_devices: Vec<crate::backend::network::HotspotDevice>,
    pub show_display_advanced_modal: bool, pub show_night_shift_modal: bool, pub night_shift_enabled: bool, pub night_shift_temp: u32,
    pub screentime_apps: Vec<crate::backend::screentime::AppUsage>, pub screentime_weekly: Vec<crate::backend::screentime::DailyUsage>,
    pub screentime_week_offset: i32, pub screentime_selected_day_idx: usize,
    pub touchpad_natural_scroll: bool, pub touchpad_tap_to_click: bool, pub touchpad_scroll_factor: f32, pub gesture_workspace_swipe: bool,
    pub mouse_sensitivity: f32, pub touchpad_sensitivity: f32, pub touchpad_drag_3fg: bool, pub numlock_by_default: bool,
    pub battery_percent: u32, pub is_charging: bool, battery_state: String, pub power_mode: String, pub battery_health: f32, pub battery_cycle_count: u32, pub battery_power_usage: f32, pub battery_remaining_time: String, pub battery_technology: String, pub battery_voltage: f32, pub battery_voltage_min_design: f32, pub battery_energy_full: f32, pub battery_energy_full_design: f32, pub battery_vendor: String, pub battery_model: String, pub hypridle_config: crate::backend::power::HypridleConfig,
    pub monitors: Vec<crate::backend::hyprland::AppMonitor>, pub keybinds: Vec<hyprland::Keybind>, pub startup_apps: Vec<hyprland::StartupApp>, pub sys_info: Option<system::SystemInfo>, pub key_repeat_rate: u32, pub key_repeat_delay: u32, pub keyboard_brightness: u32, pub swap_ctrl_alt: bool, pub keyboard_navigation: bool, pub disabled_startup_apps: std::collections::HashSet<String>,
    pub security_info: security::SecurityInfo, pub screen_lock_timeout: String, pub require_password_sleep: bool, pub password_policy: String, pub session_timeout: String, pub audit_logging_enabled: bool, pub usb_guard_enabled: bool,
    pub mako_config: crate::backend::notifications::MakoConfig, pub focus_state: crate::backend::notifications::FocusState,
    pub waybar_config: Option<crate::backend::waybar::WaybarConfig>,
    pub show_security_modal: bool, pub security_password: String, pub security_error: Option<String>, pub pending_security_action: Option<Message>,
    pub modal_transition: f32,
    pub audio: AudioService, pub display: DisplayService, pub bluetooth: BluetoothService, pub network: NetworkService, pub power: PowerService,
}

#[derive(Debug, Clone)]
pub enum Message {
    SetBluetoothAutoOn(bool),
    Navigate(Route), NavigateBack, SearchChanged(String), ToggleTheme, Tick(()), SetVolume(u32), SetBrightness(u32), ToggleMute, SetDeviceVolume(String, u32), SetSourceVolume(String, u32), ToggleDeviceMute(String, bool), SetDefaultSink(String), SetDefaultSource(String), SetCardProfile(String, String), AudioUpdateResult(Vec<crate::backend::audio::AudioDevice>, Vec<crate::backend::audio::AudioDevice>, Vec<crate::backend::audio::AudioCard>), SetBluetooth(bool), ConnectBluetooth(String), DisconnectBluetooth(String), BluetoothConnectionResult(String, bool), BluetoothAdapterUpdated(Option<crate::backend::bluetooth::BluetoothAdapter>, Vec<crate::backend::bluetooth::BluetoothDevice>), StartBluetoothDiscovery, StopBluetoothDiscovery, OpenBluetoothRenameModal, CloseBluetoothRenameModal, NewBluetoothNameChanged(String), SubmitBluetoothRename, SetWifi(bool), ScanWifi, NetworkUpdateResult(String, Vec<WifiNetwork>, Vec<String>, Vec<crate::backend::network::VpnInfo>, ConnectionDetails, bool), ConnectionDetailsResult(ConnectionDetails), SetDns(String), DnsInputChanged(String), SubmitDns, ToggleTailscale(bool), OpenNetworkModal(String), CloseNetworkModal, NetworkPasswordChanged(String), SubmitNetworkConnection, ForgetNetwork(String), ConnectToKnown(String), OpenNetworkAdvancedModal, CloseNetworkAdvancedModal, ToggleAutoJoin(String), ToggleHotspot(bool), HotspotSsidChanged(String), HotspotPasswordChanged(String), HotspotSecurityChanged(String), SaveHotspotSettings, HotspotDevicesUpdateResult(Vec<crate::backend::network::HotspotDevice>), SetResolution(String, String, String), SetScale(String, f32), SetScaleAuto(String), SetTransform(String, i32), SetPowerMode(String), SetLockTimeout(u32), SetDpmsTimeout(u32), SetSuspendTimeout(u32), SetHibernateTimeout(u32), PowerUpdateResult(crate::backend::power::BatteryInfo, String), SetKeyRepeatRate(u32), SetKeyRepeatDelay(u32), SetKeyboardBrightness(u32), ToggleSwapCtrlAlt(bool), ToggleKeyboardNavigation(bool), ToggleStartupApp(String, bool), ToggleSsh(bool), ToggleFirewall(bool), SetScreenLockTimeout(String), ToggleRequirePassword(bool), SetPasswordPolicy(String), SetSessionTimeout(String), ToggleAuditLogging(bool), ToggleUsbGuard(bool), RefreshSecurity, SecurityRefreshed(security::SecurityInfo), ScreenTimeUpdateResult(Vec<crate::backend::screentime::AppUsage>, Vec<crate::backend::screentime::DailyUsage>), SelectScreentimeDay(usize), NavigateScreentimeWeek(i32), UpdateMakoConfig(crate::backend::notifications::MakoConfig), SaveMakoConfig, ToggleFocusMode(bool, Option<String>), StartFocusTimer(u32, Option<String>), FocusStateUpdateResult(crate::backend::notifications::FocusState), SendTestNotification, OpenSecurityModal(Box<Message>), CloseSecurityModal, SecurityPasswordChanged(String), SubmitSecurityVerification, SecurityActionResult(bool), OpenDisplayAdvancedModal, CloseDisplayAdvancedModal, OpenNightShiftModal, CloseNightShiftModal, ToggleNightShift(bool), SetNightShiftTemp(u32), ToggleTouchpadNaturalScroll(bool), ToggleTouchpadTapToClick(bool), SetTouchpadScrollFactor(f32), ToggleGestureWorkspaceSwipe(bool), ToggleTouchpadDrag3fg(bool), ToggleNumlockByDefault(bool), SetMouseSensitivity(f32), SetTouchpadSensitivity(f32), EmptyTrash, OptimizeStorage, StoreInCloud, OpenStorageDetails(String),
    RestartWaybar, UpdateWaybarConfig(crate::backend::waybar::WaybarConfig), SaveWaybarConfig, WaybarUpdateResult(crate::backend::waybar::WaybarConfig),
    SetWallpaper(String),
    // Async navigation results — populated when data finishes loading after a Navigate event
    MonitorsUpdateResult(Vec<crate::backend::hyprland::AppMonitor>),
    KeybindsUpdateResult(Vec<hyprland::Keybind>),
    StartupAppsUpdateResult(Vec<hyprland::StartupApp>),
    NotificationsUpdateResult(crate::backend::notifications::MakoConfig, crate::backend::notifications::FocusState),
    InputConfigUpdateResult(crate::backend::hyprland::InputConfig),
    SystemUpdateResult(system::SystemInfo),
}

impl Default for App {
    fn default() -> Self {
        let audio = AudioService::new().unwrap(); let display = DisplayService::new().unwrap(); let bluetooth = BluetoothService::new().unwrap(); let network = NetworkService::new().unwrap(); let power = PowerService::new().unwrap();
        let volume = audio.get_sinks().ok().and_then(|sinks| sinks.iter().find(|s| s.is_default).map(|s| s.volume)).unwrap_or(0);
        let brightness = display.get_brightness().unwrap_or(0);
        let is_muted = audio.get_sinks().ok().and_then(|sinks| sinks.iter().find(|s| s.is_default).map(|s| s.is_muted)).unwrap_or(false);
        let audio_sinks = audio.get_sinks().unwrap_or_default(); let audio_sources = audio.get_sources().unwrap_or_default(); let audio_cards = audio.get_cards().unwrap_or_default();
        let wifi_on = network.is_wifi_on().unwrap_or(false); let current_ssid = network.get_current_ssid().unwrap_or_else(|_| "Not Connected".into()); let available_networks = network.get_available_networks().unwrap_or_default();
        let bat = power.get_battery_info().unwrap_or(crate::backend::power::BatteryInfo { percentage: 0, is_charging: false, state: "unknown".into(), power_now: 0.0, health: 0.0, cycle_count: 0, technology: "Unknown".into(), energy_now: 0.0, energy_full: 0.0, energy_full_design: 0.0, voltage: 0.0, voltage_min_design: 0.0, vendor: "Unknown".into(), model: "Unknown".into() });
        let power_mode = power.get_power_mode().unwrap_or("balanced".into());
        let monitors = hyprland::get_monitors().unwrap_or_default(); let keybinds = hyprland::get_keybinds().unwrap_or_default(); let startup_apps = hyprland::get_startup_apps().unwrap_or_default();
        let sys_info = system::get_system_info().ok(); let security_info = security::get_security_info();
        let today_idx = (chrono::Local::now().weekday().num_days_from_monday()) as usize;
        let input_config = hyprland::get_input_config().unwrap_or_default();
        Self {
            route: Route::Home, history: Vec::new(), search_query: String::new(), theme: MacOSTheme::Dark, transition_progress: 1.0, volume, brightness, is_muted, audio_sinks, audio_sources, audio_cards, 
            bluetooth_on: false, bluetooth_auto_on: bluetooth.is_auto_start_enabled(), bluetooth_adapter_name: "System".to_string(), bluetooth_adapter: None, bluetooth_devices: Vec::new(), is_bluetooth_scanning: false, 
            show_bluetooth_rename_modal: false, new_bluetooth_name: String::new(), connecting_device_path: None, last_tick: Instant::now(), last_data_update: Instant::now(), wifi_on, is_wifi_scanning: false, current_ssid, available_networks, dns_servers: network.get_dns_servers(), vpns: network.get_vpn_status(), new_dns_input: String::new(), show_network_modal: false, target_ssid: String::new(), network_password: String::new(), network_error: None, show_network_advanced_modal: false, auto_join_networks: vec!["TP_Link".to_string()].into_iter().collect(), connection_details: ConnectionDetails::default(), hotspot_enabled: false, hotspot_ssid: "Omarchy-Hotspot".into(), hotspot_password: "password".into(), hotspot_security: "WPA2".into(), hotspot_devices: Vec::new(), show_display_advanced_modal: false, show_night_shift_modal: false, night_shift_enabled: false, night_shift_temp: 4500, screentime_apps: crate::backend::screentime::get_running_apps_usage(), screentime_weekly: crate::backend::screentime::get_weekly_usage(0), screentime_week_offset: 0, screentime_selected_day_idx: today_idx, touchpad_natural_scroll: input_config.touchpad_natural_scroll, touchpad_tap_to_click: input_config.touchpad_tap_to_click, touchpad_scroll_factor: input_config.touchpad_scroll_factor, gesture_workspace_swipe: input_config.gesture_workspace_swipe,
            mouse_sensitivity: input_config.mouse_sensitivity, touchpad_sensitivity: input_config.touchpad_sensitivity, touchpad_drag_3fg: input_config.drag_3fg, numlock_by_default: input_config.numlock_by_default,
 battery_percent: bat.percentage, is_charging: bat.is_charging, battery_state: bat.state, power_mode, battery_health: bat.health, battery_cycle_count: bat.cycle_count, battery_power_usage: bat.power_now, battery_remaining_time: if bat.is_charging { "Charging".into() } else if bat.power_now > 0.01 { format!("{:.1}h", bat.energy_now / bat.power_now) } else { "Unknown".into() }, battery_technology: bat.technology, battery_voltage: bat.voltage, battery_voltage_min_design: bat.voltage_min_design, battery_energy_full: bat.energy_full, battery_energy_full_design: bat.energy_full_design, battery_vendor: bat.vendor, battery_model: bat.model, hypridle_config: power.get_hypridle_config(), monitors, keybinds, startup_apps, sys_info, key_repeat_rate: 25, key_repeat_delay: 600, keyboard_brightness: 50, swap_ctrl_alt: false, keyboard_navigation: true, disabled_startup_apps: std::collections::HashSet::new(),
 audit_logging_enabled: security_info.audit_logging_enabled, security_info, screen_lock_timeout: "5 minutes".to_string(), require_password_sleep: true, password_policy: "Strong".to_string(), session_timeout: "30 minutes".to_string(), usb_guard_enabled: false, mako_config: crate::backend::notifications::get_mako_config(), focus_state: crate::backend::notifications::get_focus_state(), show_security_modal: false, security_password: String::new(), security_error: None, pending_security_action: None, modal_transition: 0.0, audio, display, bluetooth, network, power,
            waybar_config: None,
        }
    }
}

impl App {
    pub fn title(&self) -> &'static str {
        match self.route {
            Route::Home => "System Settings", Route::Display => "Displays", Route::Network => "Network", Route::Hotspot => "Hotspot", Route::Bluetooth => "Bluetooth", Route::Audio => "Sound", Route::TouchpadMouse => "Touchpad & Mouse", Route::Appearance => "Appearance", Route::Waybar => "Waybar", Route::Keybinds => "Keybinds", Route::System => "General", Route::Power => "Battery & Power", Route::Startup => "Startup Apps", Route::Security => "Privacy & Security", Route::Notifications => "Notifications", Route::Focus => "Focus", Route::ScreenTime => "Screen Time", Route::Wallpaper => "Wallpaper", Route::About => "About", Route::SoftwareUpdate => "Software Update", Route::Storage => "Storage", Route::DateTime => "Date & Time", Route::LanguageRegion => "Language & Region", Route::GameMode => "Game Mode",
        }
    }

    fn save_input_settings(&self) {
        let config = crate::backend::hyprland::InputConfig {
            mouse_sensitivity: self.mouse_sensitivity,
            touchpad_sensitivity: self.touchpad_sensitivity,
            touchpad_natural_scroll: self.touchpad_natural_scroll,
            touchpad_tap_to_click: self.touchpad_tap_to_click,
            touchpad_scroll_factor: self.touchpad_scroll_factor,
            gesture_workspace_swipe: self.gesture_workspace_swipe,
            drag_3fg: self.touchpad_drag_3fg,
            numlock_by_default: self.numlock_by_default,
        };
        let _ = crate::backend::hyprland::save_input_config(&config);
    }

    pub fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Navigate(route) => {
                if self.route != route {
                    self.history.push(self.route.clone());
                    self.route = route.clone();
                    self.transition_progress = 0.0;
                    // All data loading is done asynchronously to avoid blocking the UI thread.
                    match route {
                        Route::Network => {
                            let n = self.network.clone();
                            return Task::perform(async move {
                                let ssid = n.get_current_ssid().unwrap_or_else(|_| "Not Connected".into());
                                let nets = n.get_available_networks().unwrap_or_default();
                                let dns = n.get_dns_servers();
                                let vpns = n.get_vpn_status();
                                let details = ConnectionDetails::default();
                                let powered = n.is_wifi_on().unwrap_or(false);
                                (ssid, nets, dns, vpns, details, powered)
                            }, |(ssid, nets, dns, vpns, details, powered)| {
                                Message::NetworkUpdateResult(ssid, nets, dns, vpns, details, powered)
                            });
                        },
                        Route::Display => {
                            return Task::perform(async move {
                                hyprland::get_monitors().unwrap_or_default()
                            }, |monitors| Message::MonitorsUpdateResult(monitors));
                        },
                        Route::Keybinds => {
                            return Task::perform(async move {
                                hyprland::get_keybinds().unwrap_or_default()
                            }, |keybinds| Message::KeybindsUpdateResult(keybinds));
                        },
                        Route::Power => {
                            // PowerService doesn't impl Clone; call synchronously then
                            // hand off cloneable data to the async wrapper.
                            let bat = self.power.get_battery_info().unwrap_or(crate::backend::power::BatteryInfo {
                                percentage: 0, is_charging: false, state: "unknown".into(),
                                power_now: 0.0, health: 0.0, cycle_count: 0, technology: "Unknown".into(),
                                energy_now: 0.0, energy_full: 0.0, energy_full_design: 0.0,
                                voltage: 0.0, voltage_min_design: 0.0,
                                vendor: "Unknown".into(), model: "Unknown".into(),
                            });
                            let mode = self.power.get_power_mode().unwrap_or("balanced".into());
                            return Task::perform(async move { (bat, mode) }, |(bat, mode)| Message::PowerUpdateResult(bat, mode));
                        },
                        Route::Startup => {
                            return Task::perform(async move {
                                hyprland::get_startup_apps().unwrap_or_default()
                            }, |apps| Message::StartupAppsUpdateResult(apps));
                        },
                        Route::Security => {
                            return Task::perform(async move {
                                security::get_security_info()
                            }, Message::SecurityRefreshed);
                        },
                        Route::Notifications => {
                            return Task::perform(async move {
                                let config = crate::backend::notifications::get_mako_config();
                                let state = crate::backend::notifications::get_focus_state();
                                (config, state)
                            }, |(config, state)| Message::NotificationsUpdateResult(config, state));
                        },
                        Route::Focus => {
                            return Task::perform(async move {
                                crate::backend::notifications::get_focus_state()
                            }, Message::FocusStateUpdateResult);
                        },
                        Route::ScreenTime => {
                            let offset = self.screentime_week_offset;
                            return Task::perform(async move {
                                let apps = crate::backend::screentime::get_running_apps_usage();
                                let weekly = crate::backend::screentime::get_weekly_usage(offset);
                                (apps, weekly)
                            }, |(apps, weekly)| Message::ScreenTimeUpdateResult(apps, weekly));
                        },
                        Route::Bluetooth => {
                            let bt = self.bluetooth.clone();
                            return Task::perform(async move {
                                (bt.get_adapter_info().await.ok(), bt.get_devices().await.unwrap_or_default())
                            }, |(adapter, devices)| Message::BluetoothAdapterUpdated(adapter, devices));
                        },
                        Route::TouchpadMouse => {
                            return Task::perform(async move {
                                crate::backend::hyprland::get_input_config().unwrap_or_default()
                            }, Message::InputConfigUpdateResult);
                        },
                        Route::Waybar => {
                            return Task::perform(async move {
                                crate::backend::waybar::WaybarService::load_config().ok()
                            }, |config| {
                                if let Some(c) = config {
                                    Message::WaybarUpdateResult(c)
                                } else {
                                    Message::Tick(())
                                }
                            });
                        },
                        _ => {}
                    }
                }
                Task::none()
            }
            Message::NavigateBack => { if let Some(prev) = self.history.pop() { self.route = prev; self.transition_progress = 0.0; } Task::none() }
            Message::Tick(_) => {
                let now = Instant::now();
                self.last_tick = now; 
                
                // Fast animations (16ms)
                if self.transition_progress < 1.0 { self.transition_progress = (self.transition_progress + 0.15).min(1.0); }
                let modal_visible = self.show_network_modal || self.show_network_advanced_modal || self.show_security_modal;
                if modal_visible {
                    if self.modal_transition < 1.0 { self.modal_transition = (self.modal_transition + 0.15).min(1.0); }
                } else {
                    if self.modal_transition > 0.0 { self.modal_transition = (self.modal_transition - 0.15).max(0.0); }
                }

                // Throttled data updates (every 15 seconds)
                if now.duration_since(self.last_data_update) > Duration::from_secs(15) {
                    self.last_data_update = now;
                    
                    if self.route == Route::Audio { let audio = self.audio.clone(); return Task::perform(async move { (audio.get_sinks().unwrap_or_default(), audio.get_sources().unwrap_or_default(), audio.get_cards().unwrap_or_default()) }, |(s, src, c)| Message::AudioUpdateResult(s, src, c)); }
                    if self.route == Route::Network { 
                        let n = self.network.clone(); 
                        let fetch_details = self.show_network_advanced_modal;
                        return Task::perform(async move { 
                            let ssid = n.get_current_ssid().unwrap_or_else(|_| "Not Connected".into());
                            let nets = n.get_available_networks().unwrap_or_default();
                            let dns = n.get_dns_servers();
                            let vpns = n.get_vpn_status();
                            let details = if fetch_details { n.get_connection_details().unwrap_or_default() } else { ConnectionDetails::default() };
                            let powered = n.is_wifi_on().unwrap_or(false);
                            (ssid, nets, dns, vpns, details, powered) 
                        }, |(ssid, nets, dns, vpns, details, powered)| Message::NetworkUpdateResult(ssid, nets, dns, vpns, details, powered));
                    }
                    if self.route == Route::Hotspot && self.hotspot_enabled { let n = self.network.clone(); return Task::perform(async move { n.get_hotspot_devices().unwrap_or_default() }, Message::HotspotDevicesUpdateResult); }
                    if self.route == Route::Focus || self.route == Route::Notifications { return Task::perform(async move { crate::backend::notifications::get_focus_state() }, Message::FocusStateUpdateResult); }
                }
                Task::none()
            }
            Message::NetworkUpdateResult(ssid, nets, dns, vpns, details, powered) => {
                self.current_ssid = ssid;
                self.available_networks = nets;
                self.dns_servers = dns;
                self.vpns = vpns;
                self.wifi_on = powered;
                if !details.ip_address.is_empty() || self.show_network_advanced_modal {
                    self.connection_details = details;
                }
                self.is_wifi_scanning = false;
                Task::none()
            }
            Message::ConnectionDetailsResult(d) => { self.connection_details = d; Task::none() }
            Message::ScreenTimeUpdateResult(apps, weekly) => {
                self.screentime_weekly = weekly;
                if self.screentime_selected_day_idx < self.screentime_weekly.len() {
                    let day_apps = self.screentime_weekly[self.screentime_selected_day_idx].apps.clone();
                    if !day_apps.is_empty() {
                        self.screentime_apps = day_apps;
                    } else if self.screentime_week_offset == 0 && self.screentime_selected_day_idx == (chrono::Local::now().weekday().num_days_from_monday() as usize) {
                        self.screentime_apps = apps;
                    } else {
                        self.screentime_apps = Vec::new();
                    }
                }
                Task::none()
            }
            Message::SelectScreentimeDay(idx) => {
                self.screentime_selected_day_idx = idx;
                if idx < self.screentime_weekly.len() {
                    self.screentime_apps = self.screentime_weekly[idx].apps.clone();
                }
                Task::none()
            }
            Message::NavigateScreentimeWeek(delta) => {
                self.screentime_week_offset += delta;
                let offset = self.screentime_week_offset;
                Task::perform(async move {
                    let apps = crate::backend::screentime::get_running_apps_usage();
                    let weekly = crate::backend::screentime::get_weekly_usage(offset);
                    (apps, weekly)
                }, |(apps, weekly)| Message::ScreenTimeUpdateResult(apps, weekly))
            }
            Message::UpdateMakoConfig(config) => { self.mako_config = config; Task::none() }
            Message::SaveMakoConfig => { let config = self.mako_config.clone(); Task::perform(async move { let _ = crate::backend::notifications::save_mako_config(&config); }, |_| Message::Tick(())) }
            Message::SetVolume(vol) => { self.volume = vol; if let Some(d) = self.audio_sinks.iter().find(|s| s.is_default) { let _ = self.audio.set_volume(&d.id, vol); } Task::none() }
            Message::SetBrightness(bright) => { self.brightness = bright; let _ = self.display.set_brightness(bright); Task::none() }
            Message::ToggleMute => { if let Some(d) = self.audio_sinks.iter().find(|s| s.is_default) { let _ = self.audio.toggle_mute(&d.id, true); } Task::none() }
            Message::SetDeviceVolume(id, vol) => { let _ = self.audio.set_volume(&id, vol); if let Some(d) = self.audio_sinks.iter_mut().find(|d| d.id == id) { d.volume = vol; } Task::none() }
            Message::SetSourceVolume(id, vol) => { let _ = self.audio.set_source_volume(&id, vol); if let Some(d) = self.audio_sources.iter_mut().find(|d| d.id == id) { d.volume = vol; } Task::none() }
            Message::ToggleDeviceMute(id, is_sink) => { let _ = self.audio.toggle_mute(&id, is_sink); if is_sink { if let Some(d) = self.audio_sinks.iter_mut().find(|d| d.id == id) { d.is_muted = !d.is_muted; } } else { if let Some(d) = self.audio_sources.iter_mut().find(|d| d.id == id) { d.is_muted = !d.is_muted; } } Task::none() }
            Message::SetDefaultSink(id) => { let _ = self.audio.set_default(&id, true); Task::none() }
            Message::SetDefaultSource(id) => { let _ = self.audio.set_default(&id, false); Task::none() }
            Message::SetCardProfile(c, p) => { let _ = self.audio.set_card_profile(&c, &p); Task::none() }
            Message::SetBluetooth(on) => { self.bluetooth_on = on; let bt = self.bluetooth.clone(); Task::perform(async move { let _ = bt.set_powered(on).await; }, |_| Message::Tick(())) }
            Message::SetBluetoothAutoOn(on) => { self.bluetooth_auto_on = on; let bt = self.bluetooth.clone(); let _ = bt.set_auto_start(on); Task::none() }
            Message::ConnectBluetooth(path) => { self.connecting_device_path = Some(path.clone()); let bt = self.bluetooth.clone(); let p = path.clone(); Task::perform(async move { bt.connect_device(&p).await.is_ok() }, move |s| Message::BluetoothConnectionResult(path.clone(), s)) }
            Message::DisconnectBluetooth(path) => { let bt = self.bluetooth.clone(); Task::perform(async move { let _ = bt.disconnect_device(&path).await; }, |_| Message::Tick(())) }
            Message::BluetoothConnectionResult(_, _) => { self.connecting_device_path = None; Task::none() }
            Message::BluetoothAdapterUpdated(adapter, devices) => {
                if let Some(a) = &adapter {
                    self.bluetooth_adapter_name = a.name.clone();
                    self.bluetooth_on = a.powered;
                    self.is_bluetooth_scanning = a.discovering;
                }
                self.bluetooth_adapter = adapter;
                self.bluetooth_devices = devices;
                Task::none()
            }
            Message::StartBluetoothDiscovery => { let bt = self.bluetooth.clone(); Task::perform(async move { let _ = bt.start_discovery().await; }, |_| Message::Tick(())) }
            Message::StopBluetoothDiscovery => { let bt = self.bluetooth.clone(); Task::perform(async move { let _ = bt.stop_discovery().await; }, |_| Message::Tick(())) }
            Message::OpenBluetoothRenameModal => { self.show_bluetooth_rename_modal = true; self.new_bluetooth_name = self.bluetooth_adapter_name.clone(); self.modal_transition = 0.0; Task::none() }
            Message::CloseBluetoothRenameModal => { self.show_bluetooth_rename_modal = false; Task::none() }
            Message::NewBluetoothNameChanged(n) => { self.new_bluetooth_name = n; Task::none() }
            Message::SubmitBluetoothRename => { self.show_bluetooth_rename_modal = false; let bt = self.bluetooth.clone(); let n = self.new_bluetooth_name.clone(); Task::perform(async move { let _ = bt.set_adapter_alias(&n).await; }, |_| Message::Tick(())) }
            Message::SetWifi(on) => { self.wifi_on = on; let _ = self.network.set_wifi_powered("wlan0", on); self.available_networks = self.network.get_available_networks().unwrap_or_default(); Task::none() }
            Message::ScanWifi => { self.is_wifi_scanning = true; let ns = self.network.clone(); Task::perform(async move { let _ = ns.scan(); tokio::time::sleep(Duration::from_secs(2)).await; (ns.get_current_ssid().unwrap_or_else(|_| "Not Connected".into()), ns.get_available_networks().unwrap_or_default(), ns.get_dns_servers(), ns.get_vpn_status(), ns.get_connection_details().unwrap_or_default(), ns.is_wifi_on().unwrap_or(false)) }, |(s, n, d, v, det, p)| Message::NetworkUpdateResult(s, n, d, v, det, p)) }
            Message::SetDns(_) => Task::none(),
            Message::DnsInputChanged(i) => { self.new_dns_input = i; Task::none() }
            Message::SubmitDns => { let n = self.network.clone(); let d = self.new_dns_input.clone(); Task::perform(async move { let _ = n.set_dns(&d); }, |_| Message::Tick(())) }
            Message::ToggleTailscale(a) => { let n = self.network.clone(); Task::perform(async move { let _ = n.set_tailscale_active(a); }, |_| Message::Tick(())) }
            Message::OpenNetworkModal(s) => { self.target_ssid = s; self.network_password.clear(); self.network_error = None; self.show_network_modal = true; self.modal_transition = 0.0; Task::none() }
            Message::CloseNetworkModal => { self.show_network_modal = false; Task::none() }
            Message::OpenNetworkAdvancedModal => { self.show_network_advanced_modal = true; self.modal_transition = 0.0; Task::none() }
            Message::CloseNetworkAdvancedModal => { self.show_network_advanced_modal = false; Task::none() }
            Message::ToggleAutoJoin(s) => { if self.auto_join_networks.contains(&s) { self.auto_join_networks.remove(&s); } else { self.auto_join_networks.insert(s); } Task::none() }
            Message::NetworkPasswordChanged(p) => { self.network_password = p; Task::none() }
            Message::SubmitNetworkConnection => { self.network_error = None; let pass = if self.network_password.is_empty() { None } else { Some(self.network_password.as_str()) }; match self.network.connect_to_network(&self.target_ssid, pass) { Ok(_) => { self.show_network_modal = false; self.current_ssid = self.network.get_current_ssid().unwrap_or_else(|_| "Not Connected".into()); self.available_networks = self.network.get_available_networks().unwrap_or_default(); }, Err(e) => { let m = e.to_string(); self.network_error = Some(if m.is_empty() { "Failed to connect.".into() } else { m }); } } Task::none() }
            Message::ConnectToKnown(s) => { let _ = self.network.connect_to_network(&s, None); self.current_ssid = self.network.get_current_ssid().unwrap_or_else(|_| "Not Connected".into()); self.available_networks = self.network.get_available_networks().unwrap_or_default(); Task::none() }
            Message::ForgetNetwork(s) => { let _ = self.network.forget_network(&s); self.available_networks = self.network.get_available_networks().unwrap_or_default(); if self.current_ssid == s { self.current_ssid = "Not Connected".to_string(); } Task::none() }
            Message::ToggleHotspot(e) => {
                if e && self.hotspot_password.len() < 8 { return Task::none(); }
                self.hotspot_enabled = e;
                if e {
                    let s = self.hotspot_ssid.clone();
                    let p = self.hotspot_password.clone();
                    let dev = self.network.get_wifi_device();
                    let cmd = format!(
                        "iwctl device {} set-property Mode ap && iwctl ap {} start '{}' '{}'",
                        dev, dev, s, p
                    );
                    let _ = std::process::Command::new("sh").args(["-c", &cmd]).spawn();
                } else {
                    let dev = self.network.get_wifi_device();
                    let cmd = format!("iwctl ap {} stop; iwctl device {} set-property Mode station", dev, dev);
                    let _ = std::process::Command::new("sh").args(["-c", &cmd]).spawn();
                }
                Task::none()
            }
            Message::HotspotSsidChanged(s) => { self.hotspot_ssid = s; Task::none() }
            Message::HotspotPasswordChanged(p) => { self.hotspot_password = p; Task::none() }
            Message::HotspotSecurityChanged(s) => { self.hotspot_security = s; Task::none() }
            Message::SetResolution(m, r, f) => { let _ = hyprland::set_resolution(&m, &r, &f); self.monitors = hyprland::get_monitors().unwrap_or_default(); Task::none() }
            Message::SetScale(m, s) => { let _ = hyprland::set_scale(&m, s); self.monitors = hyprland::get_monitors().unwrap_or_default(); Task::none() }
            Message::SetScaleAuto(m) => { let _ = hyprland::set_scale_auto(&m); self.monitors = hyprland::get_monitors().unwrap_or_default(); Task::none() }
            Message::SetTransform(m, t) => { let _ = hyprland::set_transform(&m, t); self.monitors = hyprland::get_monitors().unwrap_or_default(); Task::none() }
            Message::SetPowerMode(m) => { let _ = self.power.set_power_mode(&m); self.power_mode = m; Task::none() }
            Message::SetLockTimeout(v) => { self.hypridle_config.lock_timeout = v; let _ = self.power.set_hypridle_config(&self.hypridle_config); Task::none() }
            Message::SetDpmsTimeout(v) => { self.hypridle_config.dpms_timeout = v; let _ = self.power.set_hypridle_config(&self.hypridle_config); Task::none() }
            Message::SetSuspendTimeout(v) => { self.hypridle_config.suspend_timeout = v; let _ = self.power.set_hypridle_config(&self.hypridle_config); Task::none() }
            Message::SetHibernateTimeout(v) => { self.hypridle_config.hibernate_timeout = v; let _ = self.power.set_hypridle_config(&self.hypridle_config); Task::none() }
            Message::PowerUpdateResult(bat, mode) => { self.battery_percent = bat.percentage; self.is_charging = bat.is_charging; self.battery_state = bat.state; self.power_mode = mode; self.battery_health = bat.health; self.battery_cycle_count = bat.cycle_count; self.battery_power_usage = bat.power_now; self.battery_remaining_time = if bat.is_charging { "Charging".into() } else if bat.power_now > 0.01 { format!("{:.1}h", bat.energy_now / bat.power_now) } else { "Unknown".into() }; self.battery_technology = bat.technology; self.battery_voltage = bat.voltage; self.battery_voltage_min_design = bat.voltage_min_design; self.battery_energy_full = bat.energy_full; self.battery_energy_full_design = bat.energy_full_design; self.battery_vendor = bat.vendor; self.battery_model = bat.model; Task::none() }

            Message::SetKeyRepeatRate(r) => { self.key_repeat_rate = r; Task::none() }
            Message::SetKeyRepeatDelay(d) => { self.key_repeat_delay = d; Task::none() }
            Message::SetKeyboardBrightness(b) => { let _ = hyprland::set_keyboard_brightness(b); self.keyboard_brightness = b; Task::none() }
            Message::ToggleSwapCtrlAlt(e) => { let _ = hyprland::set_swap_ctrl_alt(e); self.swap_ctrl_alt = e; Task::none() }
            Message::ToggleKeyboardNavigation(e) => { self.keyboard_navigation = e; Task::none() }
            Message::ToggleStartupApp(id, e) => { if e { self.disabled_startup_apps.remove(&id); } else { self.disabled_startup_apps.insert(id); } Task::none() }
            Message::ToggleSsh(e) => { let _ = std::process::Command::new("sudo").args(["systemctl", if e { "enable" } else { "disable" }, "--now", "sshd"]).status(); Task::none() }
            Message::ToggleFirewall(e) => { let _ = std::process::Command::new("sudo").args(["systemctl", if e { "enable" } else { "disable" }, "--now", "ufw"]).status(); Task::none() }
            Message::SetScreenLockTimeout(v) => { self.screen_lock_timeout = v; Task::none() }
            Message::ToggleRequirePassword(e) => { self.require_password_sleep = e; Task::none() }
            Message::SetPasswordPolicy(p) => { self.password_policy = p; Task::none() }
            Message::SetSessionTimeout(v) => { self.session_timeout = v; Task::none() }
            Message::ToggleAuditLogging(e) => { self.audit_logging_enabled = e; Task::none() }
            Message::ToggleUsbGuard(e) => { self.usb_guard_enabled = e; Task::none() }
            Message::RefreshSecurity => { Task::perform(async move { security::get_security_info() }, Message::SecurityRefreshed) }
            Message::SecurityRefreshed(info) => { self.security_info = info; Task::none() }
            Message::ToggleFocusMode(e, tag) => { let t = tag.clone(); let _ = crate::backend::notifications::set_focus_mode(e, t.as_deref()); Task::none() }
            Message::StartFocusTimer(m, tag) => { let t = tag.clone(); let _ = crate::backend::notifications::set_focus_timer(m, t.as_deref()); Task::none() }
            Message::FocusStateUpdateResult(s) => { self.focus_state = s; Task::none() }
            Message::SendTestNotification => { let _ = std::process::Command::new("notify-send").args(["System Settings", "This is a test notification."]).spawn(); Task::none() }
            Message::OpenSecurityModal(m) => { self.pending_security_action = Some(*m); self.security_password.clear(); self.security_error = None; self.show_security_modal = true; self.modal_transition = 0.0; Task::none() }
            Message::CloseSecurityModal => { self.show_security_modal = false; Task::none() }
            Message::SecurityPasswordChanged(p) => { self.security_password = p; Task::none() }
            Message::SubmitSecurityVerification => { if security::verify_password(&self.security_password) { let msg = self.pending_security_action.take(); self.show_security_modal = false; if let Some(m) = msg { return self.update(m); } } else { self.security_error = Some("Incorrect password.".into()); } Task::none() }
            Message::SecurityActionResult(_) => Task::none(),
            Message::OpenDisplayAdvancedModal => { self.show_display_advanced_modal = true; Task::none() }
            Message::CloseDisplayAdvancedModal => { self.show_display_advanced_modal = false; Task::none() }
            Message::OpenNightShiftModal => { self.show_night_shift_modal = true; Task::none() }
            Message::CloseNightShiftModal => { self.show_night_shift_modal = false; Task::none() }
            Message::ToggleNightShift(e) => { self.night_shift_enabled = e; Task::none() }
            Message::SetNightShiftTemp(v) => { self.night_shift_temp = v; Task::none() }
            Message::ToggleTouchpadNaturalScroll(e) => { let _ = hyprland::set_touchpad_natural_scroll(e); self.touchpad_natural_scroll = e; self.save_input_settings(); Task::none() }
            Message::ToggleTouchpadTapToClick(e) => { let _ = hyprland::set_touchpad_tap_to_click(e); self.touchpad_tap_to_click = e; self.save_input_settings(); Task::none() }
            Message::SetTouchpadScrollFactor(v) => { let _ = hyprland::set_touchpad_scroll_factor(v); self.touchpad_scroll_factor = v; self.save_input_settings(); Task::none() }
            Message::ToggleGestureWorkspaceSwipe(e) => { let _ = hyprland::set_gesture_workspace_swipe(e); self.gesture_workspace_swipe = e; self.save_input_settings(); Task::none() }
            Message::ToggleTouchpadDrag3fg(e) => { self.touchpad_drag_3fg = e; self.save_input_settings(); Task::none() }
            Message::ToggleNumlockByDefault(e) => { self.numlock_by_default = e; self.save_input_settings(); Task::none() }
            Message::SetMouseSensitivity(v) => { self.mouse_sensitivity = v; self.save_input_settings(); Task::none() }
            Message::SetTouchpadSensitivity(v) => { self.touchpad_sensitivity = v; self.save_input_settings(); Task::none() }
            Message::InputConfigUpdateResult(config) => {
                self.touchpad_natural_scroll = config.touchpad_natural_scroll;
                self.touchpad_tap_to_click = config.touchpad_tap_to_click;
                self.touchpad_scroll_factor = config.touchpad_scroll_factor;
                self.gesture_workspace_swipe = config.gesture_workspace_swipe;
                self.touchpad_drag_3fg = config.drag_3fg;
                self.numlock_by_default = config.numlock_by_default;
                self.mouse_sensitivity = config.mouse_sensitivity;
                self.touchpad_sensitivity = config.touchpad_sensitivity;
                Task::none()
            }
            Message::EmptyTrash => {
                Task::perform(async move {
                    let _ = crate::backend::system::empty_trash();
                    crate::backend::system::get_system_info().ok()
                }, |res| {
                    if let Some(info) = res {
                        Message::SystemUpdateResult(info)
                    } else {
                        Message::Tick(())
                    }
                })
            }
            Message::OptimizeStorage => {
                Task::perform(async move {
                    let _ = crate::backend::system::optimize_storage();
                    crate::backend::system::get_system_info().ok()
                }, |res| {
                    if let Some(info) = res {
                        Message::SystemUpdateResult(info)
                    } else {
                        Message::Tick(())
                    }
                })
            }
            Message::StoreInCloud => {
                let _ = std::process::Command::new("xdg-open").arg("https://cloud.omarchy.org").spawn();
                Task::none()
            }
            Message::OpenStorageDetails(cat) => {
                let _ = std::process::Command::new("notify-send").args(["Storage", &format!("Opening details for {}", cat)]).spawn();
                Task::none()
            }
            Message::MonitorsUpdateResult(monitors) => { self.monitors = monitors; Task::none() }
            Message::KeybindsUpdateResult(keybinds) => { self.keybinds = keybinds; Task::none() }
            Message::StartupAppsUpdateResult(apps) => { self.startup_apps = apps; Task::none() }
            Message::NotificationsUpdateResult(config, state) => { self.mako_config = config; self.focus_state = state; Task::none() }
            Message::RestartWaybar => { let _ = crate::backend::waybar::WaybarService::restart(); Task::none() }
            Message::UpdateWaybarConfig(config) => { self.waybar_config = Some(config); Task::none() }
            Message::SaveWaybarConfig => {
                if let Some(config) = &self.waybar_config {
                    let config = config.clone();
                    return Task::perform(async move {
                        crate::backend::waybar::WaybarService::save_config(&config).ok()
                    }, |_| Message::Tick(()));
                }
                Task::none()
            }
            Message::WaybarUpdateResult(config) => { self.waybar_config = Some(config); Task::none() }
            Message::SetWallpaper(path) => {
                let _ = std::process::Command::new("omarchy-set-wallpaper").arg(path).spawn();
                Task::none()
            }
            _ => Task::none()
        }
    }

    pub fn view(&self) -> Element<'_, Message> {
        let theme = &self.theme;
        
        let main_view = container(row![
            sidebar::view(self),
            container(match self.route {
                Route::Home => homepage::view(self),
                Route::Display => pages::display::view(self),
                Route::Network => pages::network::view(self),
                Route::Hotspot => pages::hotspot::view(self),
                Route::Bluetooth => pages::bluetooth::view(self),
                Route::Audio => pages::audio::view(self),
                Route::TouchpadMouse => pages::touchpad_mouse::view(self),
                Route::Appearance => pages::appearance::view(self),
                Route::Keybinds => pages::keybinds::view(self),
                Route::System => pages::system::view(self),
                Route::Power => pages::power::view(self),
                Route::Startup => pages::startup::view(self),
                Route::Security => pages::security::view(self),
                Route::Notifications => pages::notifications::view(self),
                Route::Focus => pages::focus::view(self),
                Route::ScreenTime => pages::screentime::view(self),
                Route::Wallpaper => pages::wallpaper::view(self),
                Route::About => pages::about::view(self),
                Route::SoftwareUpdate => pages::software_update::view(self),
                Route::Storage => pages::storage::view(self),
                Route::DateTime => pages::date_time::view(self),
                Route::LanguageRegion => pages::language_region::view(self),
                Route::GameMode => pages::game_mode::view(self),
                Route::Waybar => pages::waybar::view(self),
            })
            .width(Length::Fill)
            .height(Length::Fill)
            .style(move |_| container::Style { background: Some(iced::Background::Color(theme.bg())), ..Default::default() })
        ])
        .width(Length::Fill)
        .height(Length::Fill);

        let mut layers = vec![main_view.into()];

        if self.modal_transition > 0.01 {
            let overlay = container(iced::widget::Space::with_width(Length::Fill))
                .width(Length::Fill)
                .height(Length::Fill)
                .style(move |_| container::Style { 
                    background: Some(iced::Background::Color(iced::Color::from_rgba(0.0, 0.0, 0.0, 0.4 * self.modal_transition))),
                    ..Default::default()
                });
            
            layers.push(overlay.into());

            let modal_content = if self.show_network_modal { Some(self.view_network_modal()) }
                else if self.show_network_advanced_modal { Some(self.view_network_advanced_modal()) }
                else if self.show_security_modal { Some(self.view_security_modal()) }
                else { None };

            if let Some(modal) = modal_content {
                let animated_modal = container(modal)
                    .width(Length::Fill)
                    .height(Length::Fill)
                    .center_x(Length::Fill)
                    .center_y(Length::Fill)
                    .padding(iced::Padding { top: 100.0 * (1.0 - self.modal_transition), right: 0.0, bottom: 0.0, left: 0.0 });
                layers.push(animated_modal.into());
            }
        }

        stack(layers).into()
    }

    fn view_network_modal(&self) -> Element<'_, Message> {
        use iced::widget::{button, column, container, row, text, text_input};
        use iced::{Length, Color};
        let theme = &self.theme;
        let mut mc = column![
            text(format!("Connect to '{}'", self.target_ssid)).size(18).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            text_input("Password", &self.network_password).secure(true).on_input(Message::NetworkPasswordChanged).on_submit(Message::SubmitNetworkConnection).padding(12)
        ].spacing(20);
        
        if let Some(err) = &self.network_error { 
            mc = mc.push(text(err).size(12).style(|_| text::Style { color: Some(Color::from_rgb8(255, 59, 48)), ..Default::default() })); 
        }
        
        mc = mc.push(row![
            button(text("Cancel").center()).width(Length::Fill).padding(10).on_press(Message::CloseNetworkModal).style(move |_, _| { 
                let mut s = button::Style::default(); 
                s.background = Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) })); 
                s.text_color = theme.text_primary(); 
                s.border.radius = 8.0.into(); 
                s 
            }), 
            button(text("Connect").center()).width(Length::Fill).padding(10).on_press(Message::SubmitNetworkConnection).style(move |_, _| { 
                let mut s = button::Style::default(); 
                s.background = Some(iced::Background::Color(theme.accent())); 
                s.text_color = Color::WHITE; 
                s.border.radius = 8.0.into(); 
                s 
            })
        ].spacing(12));

        container(mc)
            .padding(28)
            .width(Length::Fixed(400.0))
            .style(move |_| container::Style { 
                background: Some(iced::Background::Color(theme.card_bg())), 
                border: iced::Border { color: theme.border(), width: 0.5, radius: 12.0.into() }, 
                shadow: iced::Shadow { color: Color::from_rgba(0.0, 0.0, 0.0, 0.3), offset: iced::Vector::new(0.0, 20.0), blur_radius: 60.0 }, 
                ..Default::default() 
            })
            .into()
    }

    fn view_network_advanced_modal(&self) -> Element<'_, Message> {
        use iced::widget::{button, column, container, row, text, text_input, svg, Space, toggler, scrollable};
        use iced::{Length, Color};
        use crate::ui::icons;
        use crate::theme::card;
        let theme = &self.theme;
        
        let is_auto_join = self.auto_join_networks.contains(&self.current_ssid);
        
        let wifi_settings = column![
            row![
                text("Auto-Join").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(is_auto_join).on_toggle(move |_| Message::ToggleAutoJoin(self.current_ssid.clone())),
            ].padding([10, 15]),
            container(iced::widget::horizontal_rule(1)).padding([0, 10]),
            button(
                text("Forget This Network").size(14).style(move |_: &iced::Theme| text::Style { color: Some(Color::from_rgb8(255, 69, 58)), ..Default::default() })
            )
            .on_press(Message::ForgetNetwork(self.current_ssid.clone()))
            .padding([12, 15])
            .style(button::text)
        ];

        let connection_info = column![
            row![
                svg(icons::icon_info()).width(16).height(16).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_primary()) }),
                text("Connection Information").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })
            ].spacing(10).padding([10, 15]),
            container(iced::widget::horizontal_rule(1)).padding([0, 10]),
            column![
                row![text("IP Address:").size(13).width(Length::Fixed(120.0)).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }), text(&self.connection_details.ip_address).size(13).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].padding([8, 15]),
                row![text("Router:").size(13).width(Length::Fixed(120.0)).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }), text(&self.connection_details.router).size(13).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].padding([8, 15]),
                row![text("Security:").size(13).width(Length::Fixed(120.0)).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }), text(&self.connection_details.security).size(13).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].padding([8, 15]),
                row![text("Signal:").size(13).width(Length::Fixed(120.0)).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }), text(format!("{} dBm", self.connection_details.signal_strength)).size(13).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].padding([8, 15]),
            ]
        ];

        let dns_s = column![row![svg(icons::icon_globe()).width(18).height(18).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_primary()) }), text("DNS Servers").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].spacing(10).padding([10, 15]), container(iced::widget::horizontal_rule(1)).padding([0, 10]), column(self.dns_servers.iter().map(|dns| { row![text(dns).size(13).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }), svg(icons::icon_check()).width(12).height(12).style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(52, 199, 89)) })].padding([8, 15]).into() }).collect::<Vec<_>>()), row![text_input("Add DNS", &self.new_dns_input).on_input(Message::DnsInputChanged).on_submit(Message::SubmitDns).padding(8).width(Length::Fill), button(text("Apply").size(12)).padding([6, 12]).on_press(Message::SubmitDns).style(move |_, _| { let mut s = button::Style::default(); s.background = Some(iced::Background::Color(theme.accent())); s.text_color = Color::WHITE; s.border.radius = 8.0.into(); s })].spacing(10).padding([10, 15]).align_y(iced::Alignment::Center)];
        let vpn_s = column![container(text("VPN & Network Security").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })).padding([10, 15]), container(iced::widget::horizontal_rule(1)).padding([0, 10]), column(self.vpns.iter().map(|v| { let ctrl: Element<'_, Message> = if v.name.contains("Tailscale") { toggler(v.is_running).on_toggle(Message::ToggleTailscale).into() } else if v.is_running { text("Connected").size(12).style(move |_: &iced::Theme| text::Style { color: Some(Color::from_rgb8(52, 199, 89)), ..Default::default() }).into() } else { text("Not Running").size(12).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }).into() }; row![column![text(&v.name).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }), text(&v.status).size(11).style(move |_: &iced::Theme| text::Style { color: Some(if v.is_running { Color::from_rgb8(52, 199, 89) } else { theme.text_secondary() }), ..Default::default() })].width(Length::Fill).spacing(2), ctrl].align_y(iced::Alignment::Center).padding([10, 15]).into() }).collect::<Vec<_>>())];
        
        let mc = column![
            row![
                text(format!("Details for {}", self.current_ssid)).size(20).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }), 
                button(text("Done").size(13)).padding([6, 15]).on_press(Message::CloseNetworkAdvancedModal).style(move |_, _| { 
                    let mut s = button::Style::default(); 
                    s.background = Some(iced::Background::Color(theme.accent())); 
                    s.text_color = Color::WHITE; 
                    s.border.radius = 8.0.into(); 
                    s 
                })
            ].align_y(iced::Alignment::Center).padding(iced::Padding { top: 0.0, right: 0.0, bottom: 15.0, left: 0.0 }), 
            scrollable(column![card(theme, wifi_settings), Space::with_height(20), card(theme, connection_info), Space::with_height(20), card(theme, dns_s), Space::with_height(20), card(theme, vpn_s)].width(Length::Fill)).height(Length::Fixed(450.0))
        ].padding(25);

        container(mc)
            .width(Length::Fixed(550.0))
            .style(move |_| container::Style { 
                background: Some(iced::Background::Color(theme.card_bg())), 
                border: iced::Border { color: theme.border(), width: 0.5, radius: 16.0.into() }, 
                shadow: iced::Shadow { color: Color::from_rgba(0.0, 0.0, 0.0, 0.3), offset: iced::Vector::new(0.0, 20.0), blur_radius: 60.0 }, 
                ..Default::default() 
            })
            .into()
    }

    fn view_security_modal(&self) -> Element<'_, Message> {
        use iced::widget::{button, column, container, row, text, text_input, svg};
        use iced::{Alignment, Length, Color};
        use crate::ui::icons;
        let theme = &self.theme;
        let action_text = match &self.pending_security_action { Some(Message::ToggleSsh(e)) => if *e { "enable SSH" } else { "disable SSH" }, Some(Message::ToggleFirewall(e)) => if *e { "enable Firewall" } else { "disable Firewall" }, Some(Message::ToggleAuditLogging(e)) => if *e { "enable Audit" } else { "disable Audit" }, _ => "perform action" };
        let mc = column![
            row![svg(icons::icon_lock()).width(24).height(24).style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(255, 149, 0)) }), text("Authentication").size(18).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() })].spacing(12).align_y(Alignment::Center), 
            container(text(format!("Requires password to {}.", action_text)).size(13).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 0.0, right: 0.0, bottom: 0.0, left: 0.0 }), 
            text_input("Password", &self.security_password).secure(true).on_input(Message::SecurityPasswordChanged).on_submit(Message::SubmitSecurityVerification).padding(12)
        ].spacing(20);
        
        let mut mc = if let Some(e) = &self.security_error { mc.push(text(e).size(12).style(|_| text::Style { color: Some(Color::from_rgb8(255, 59, 48)), ..Default::default() })) } else { mc };
        
        mc = mc.push(row![
            button(text("Cancel").center()).width(Length::Fill).padding(10).on_press(Message::CloseSecurityModal).style(move |_, _| { 
                let mut s = button::Style::default(); 
                s.background = Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) })); 
                s.text_color = theme.text_primary(); 
                s.border.radius = 8.0.into(); 
                s 
            }), 
            button(text("Authenticate").center()).width(Length::Fill).padding(10).on_press(Message::SubmitSecurityVerification).style(move |_, _| { 
                let mut s = button::Style::default(); 
                s.background = Some(iced::Background::Color(theme.accent())); 
                s.text_color = Color::WHITE; 
                s.border.radius = 8.0.into(); 
                s 
            })
        ].spacing(12));

        container(mc)
            .padding(28)
            .width(Length::Fixed(400.0))
            .style(move |_| container::Style { 
                background: Some(iced::Background::Color(theme.card_bg())), 
                border: iced::Border { color: theme.border(), width: 0.5, radius: 12.0.into() }, 
                shadow: iced::Shadow { color: Color::from_rgba(0.0, 0.0, 0.0, 0.3), offset: iced::Vector::new(0.0, 20.0), blur_radius: 60.0 }, 
                ..Default::default() 
            })
            .into()
    }

    pub fn subscription(&self) -> Subscription<Message> {
        // Use fast 16ms timer only when animation is actively running.
        // When idle (no transition, no modal animation), drop to a slow 15s tick
        // so the UI thread is not spun needlessly — this is the main fix for
        // heavy CPU use and sluggish scrolling.
        let animating = self.transition_progress < 1.0
            || (self.modal_transition > 0.0 && self.modal_transition < 1.0);
        let modal_open = self.show_network_modal
            || self.show_network_advanced_modal
            || self.show_security_modal;

        if animating || modal_open {
            iced::time::every(Duration::from_millis(16)).map(|_| Message::Tick(()))
        } else {
            iced::time::every(Duration::from_secs(15)).map(|_| Message::Tick(()))
        }
    }
}
