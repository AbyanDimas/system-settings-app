use iced::widget::svg::Handle;
use once_cell::sync::Lazy;

// ─── Cached SVG Handles ───────────────────────────────────────────────────────
// Each icon is parsed ONCE at first use and then cloned cheaply (Arc pointer).
// This replaces the previous pattern of `Handle::from_memory(...)` being called
// on every `view()` invocation (62×/sec when the timer was always-on).

static ICON_SEARCH: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/></svg>"#.as_bytes())
});
pub fn icon_search() -> Handle { ICON_SEARCH.clone() }

static ICON_WIFI: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 13a10 10 0 0 1 14 0"/><path d="M8.5 16.5a5 5 0 0 1 7 0"/><path d="M2 9a15 15 0 0 1 20 0"/><line x1="12" x2="12.01" y1="20" y2="20"/></svg>"#.as_bytes())
});
pub fn icon_wifi() -> Handle { ICON_WIFI.clone() }

static ICON_WIFI_OFF: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="2" x2="22" y1="2" y2="22"/><path d="M8.5 16.5a5 5 0 0 1 7 0"/><path d="M2 8.82a15 15 0 0 1 18.17 11.13"/><path d="M10.66 5c4.01.36 7.6 2.31 10.17 5.33"/><line x1="12" x2="12.01" y1="20" y2="20"/></svg>"#.as_bytes())
});
pub fn icon_wifi_off() -> Handle { ICON_WIFI_OFF.clone() }

static ICON_WIFI_LOCKED: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 20a2 2 0 1 0 0-4 2 2 0 0 0 0 4Z"/><path d="M5 13a10 10 0 0 1 14 0"/><path d="M2 9a15 15 0 0 1 20 0"/><rect width="8" height="5" x="8" y="16" rx="1"/><path d="M10 16v-2a2 2 0 1 1 4 0v2"/></svg>"#.as_bytes())
});
pub fn icon_wifi_locked() -> Handle { ICON_WIFI_LOCKED.clone() }

// Signal strength icons are a special case: they vary by strength value.
// We cache each of the 5 variants statically.
static ICON_SIGNAL_4: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/><path d="M7 20v-4"/><path d="M12 20v-8"/><path d="M17 20V8"/><path d="M22 20V4"/></svg>"#.as_bytes())
});
static ICON_SIGNAL_3: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/><path d="M7 20v-4"/><path d="M12 20v-8"/><path d="M17 20V8"/></svg>"#.as_bytes())
});
static ICON_SIGNAL_2: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/><path d="M7 20v-4"/><path d="M12 20v-8"/></svg>"#.as_bytes())
});
static ICON_SIGNAL_1: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/><path d="M7 20v-4"/></svg>"#.as_bytes())
});
static ICON_SIGNAL_0: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/></svg>"#.as_bytes())
});

pub fn icon_signal_strength(strength: i32) -> Handle {
    let bars = if strength > -50 { 4 } else if strength > -60 { 3 } else if strength > -70 { 2 } else if strength > -80 { 1 } else { 0 };
    match bars {
        4 => ICON_SIGNAL_4.clone(),
        3 => ICON_SIGNAL_3.clone(),
        2 => ICON_SIGNAL_2.clone(),
        1 => ICON_SIGNAL_1.clone(),
        _ => ICON_SIGNAL_0.clone(),
    }
}

pub fn icon_network() -> Handle { icon_wifi() }

static ICON_BLUETOOTH: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m7 7 10 10-5 5V2l5 5L7 17"/></svg>"#.as_bytes())
});
pub fn icon_bluetooth() -> Handle { ICON_BLUETOOTH.clone() }
pub fn icon_bluetooth_off() -> Handle { ICON_BLUETOOTH.clone() }

static ICON_HOTSPOT: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/></svg>"#.as_bytes())
});
pub fn icon_hotspot() -> Handle { ICON_HOTSPOT.clone() }

static ICON_BELL: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 8a6 6 0 0 1 12 0c0 7 3 9 3 9H3s3-2 3-9"/><path d="M10.3 21a1.94 1.94 0 0 0 3.4 0"/></svg>"#.as_bytes())
});
pub fn icon_bell() -> Handle { ICON_BELL.clone() }

static ICON_MOON: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg>"#.as_bytes())
});
pub fn icon_moon() -> Handle { ICON_MOON.clone() }

static ICON_HOURGLASS: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 22h14"/><path d="M5 2h14"/><path d="M17 22v-4.17a2 2 0 0 0-.59-1.42L12 12l-4.41 4.41a2 2 0 0 0-.59 1.42V22"/><path d="M7 2v4.17a2 2 0 0 0 .59 1.42L12 12l4.41-4.41a2 2 0 0 0 .59-1.42V2"/></svg>"#.as_bytes())
});
pub fn icon_hourglass() -> Handle { ICON_HOURGLASS.clone() }

static ICON_SYSTEM: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/><circle cx="12" cy="12" r="3"/></svg>"#.as_bytes())
});
pub fn icon_system() -> Handle { ICON_SYSTEM.clone() }

static ICON_SHARE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8"/><polyline points="16 6 12 2 8 6"/><line x1="12" x2="12" y1="2" y2="15"/></svg>"#.as_bytes())
});
pub fn icon_share() -> Handle { ICON_SHARE.clone() }

static ICON_ACCESSIBILITY: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="16" cy="4" r="1"/><path d="m18 19 1-7-6 1"/><path d="m5 8 3-3 5.5 3-2.36 3.5"/><path d="M4.24 14.5a5 5 0 0 0 6.88 6"/><path d="M13.76 17.5a5 5 0 0 0-6.88-6"/></svg>"#.as_bytes())
});
pub fn icon_accessibility() -> Handle { ICON_ACCESSIBILITY.clone() }

static ICON_GAMEPAD: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="6" x2="10" y1="12" y2="12"/><line x1="8" x2="8" y1="10" y2="14"/><rect width="20" height="12" x="2" y="6" rx="2"/><circle cx="15.5" cy="10.5" r=".5" fill="currentColor"/><circle cx="17.5" cy="12.5" r=".5" fill="currentColor"/></svg>"#.as_bytes())
});
pub fn icon_gamepad() -> Handle { ICON_GAMEPAD.clone() }

static ICON_DISPLAY: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="14" x="2" y="3" rx="2"/><line x1="12" x2="12" y1="17" y2="21"/><line x1="7" x2="17" y1="21" y2="21"/></svg>"#.as_bytes())
});
pub fn icon_display() -> Handle { ICON_DISPLAY.clone() }

static ICON_AUDIO: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M11 5 6 9H2v6h4l5 4V5Z"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/></svg>"#.as_bytes())
});
pub fn icon_audio() -> Handle { ICON_AUDIO.clone() }
pub fn icon_volume_mute() -> Handle { ICON_AUDIO.clone() }
pub fn icon_volume_low() -> Handle { ICON_AUDIO.clone() }
pub fn icon_volume_mid() -> Handle { ICON_AUDIO.clone() }
pub fn icon_volume_high() -> Handle { ICON_AUDIO.clone() }

static ICON_POWER: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 7.1a9 9 0 1 1-12 0"/><line x1="12" x2="12" y1="2" y2="12"/></svg>"#.as_bytes())
});
pub fn icon_power() -> Handle { ICON_POWER.clone() }

static ICON_SHIELD: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>"#.as_bytes())
});
pub fn icon_shield() -> Handle { ICON_SHIELD.clone() }

static ICON_SHIELD_CHECK: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/><path d="m9 12 2 2 4-4"/></svg>"#.as_bytes())
});
pub fn icon_shield_check() -> Handle { ICON_SHIELD_CHECK.clone() }

static ICON_APPEARANCE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20"/><path d="M2 12h20"/></svg>"#.as_bytes())
});
pub fn icon_appearance() -> Handle { ICON_APPEARANCE.clone() }

static ICON_IMAGE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="18" height="18" x="3" y="3" rx="2" ry="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-3.086-3.086a2 2 0 0 0-2.828 0L6 21"/></svg>"#.as_bytes())
});
pub fn icon_image() -> Handle { ICON_IMAGE.clone() }

static ICON_TOUCHPAD: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="16" x="2" y="4" rx="2" ry="2"/><line x1="2" x2="22" y1="14" y2="14"/><line x1="12" x2="12" y1="14" y2="20"/></svg>"#.as_bytes())
});
pub fn icon_touchpad() -> Handle { ICON_TOUCHPAD.clone() }

static ICON_ROCKET: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4.5 16.5c-1.5 1.26-2 5-2 5s3.74-.5 5-2c.71-.84.7-2.13-.09-2.91a2.18 2.18 0 0 0-2.91-.09Z"/><path d="m12 15-3-3a22 22 0 0 1 2-3.95A12.88 2.88 0 0 1 22 2c0 2.72-.78 7.5-6 11a22.35 22.35 0 0 1-4 2Z"/><path d="M9 12H4s.55-3.03 2-5c1.62-2.2 5-3 5-3"/><path d="M12 15v5s3.03-.55 5-2c2.2-1.62 3-5 3-5"/></svg>"#.as_bytes())
});
pub fn icon_rocket() -> Handle { ICON_ROCKET.clone() }

static ICON_KEYBINDS: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="16" x="2" y="4" rx="2"/><path d="M6 8h.01"/><path d="M10 8h.01"/><path d="M14 8h.01"/><path d="M18 8h.01"/><path d="M6 12h.01"/><path d="M10 12h.01"/><path d="M14 12h.01"/><path d="M18 12h.01"/><path d="M7 16h10"/></svg>"#.as_bytes())
});
pub fn icon_keybinds() -> Handle { ICON_KEYBINDS.clone() }

static ICON_REFRESH: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M3 21v-5h5"/></svg>"#.as_bytes())
});
pub fn icon_refresh() -> Handle { ICON_REFRESH.clone() }

static ICON_EDIT: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 3a2.85 2.83 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5Z"/><path d="m15 5 4 4"/></svg>"#.as_bytes())
});
pub fn icon_edit() -> Handle { ICON_EDIT.clone() }

static ICON_LOCK: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="18" height="11" x="3" y="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>"#.as_bytes())
});
pub fn icon_lock() -> Handle { ICON_LOCK.clone() }

static ICON_MIC: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z"/><path d="M19 10v2a7 7 0 0 1-14 0v-2"/><line x1="12" x2="12" y1="19" y2="22"/></svg>"#.as_bytes())
});
pub fn icon_mic() -> Handle { ICON_MIC.clone() }

static ICON_MIC_MUTE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="2" x2="22" y1="2" y2="22"/><path d="M12 2a3 3 0 0 0-3 3v2"/><path d="M9.01 9.01V12a3 3 0 0 0 5.98 0V11"/><path d="M19 10v2a7 7 0 0 1-1.16 3.84"/><path d="M8.84 3.16A3 3 0 0 1 12 2"/><path d="M5 10v2a7 7 0 0 0 10.84 5.84"/><line x1="12" x2="12" y1="19" y2="22"/></svg>"#.as_bytes())
});
pub fn icon_mic_mute() -> Handle { ICON_MIC_MUTE.clone() }

static ICON_CHEVRON_RIGHT: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m9 18 6-6-6-6"/></svg>"#.as_bytes())
});
pub fn icon_chevron_right() -> Handle { ICON_CHEVRON_RIGHT.clone() }

static ICON_SMARTPHONE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="14" height="20" x="5" y="2" rx="2" ry="2"/><path d="M12 18h.01"/></svg>"#.as_bytes())
});
pub fn icon_smartphone() -> Handle { ICON_SMARTPHONE.clone() }

static ICON_GLOBE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20"/><path d="M2 12h20"/></svg>"#.as_bytes())
});
pub fn icon_globe() -> Handle { ICON_GLOBE.clone() }

static ICON_USERS: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>"#.as_bytes())
});
pub fn icon_users() -> Handle { ICON_USERS.clone() }

static ICON_USER: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>"#.as_bytes())
});
pub fn icon_user() -> Handle { ICON_USER.clone() }

static ICON_CROWN: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h20"/><path d="m4 20 4-12 4 6 4-6 4 12"/></svg>"#.as_bytes())
});
pub fn icon_crown() -> Handle { ICON_CROWN.clone() }

static ICON_BOOK: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5v-15A2.5 2.5 0 0 1 6.5 2H20v20H6.5a2.5 2.5 0 0 1-2.5-2.5Z"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2Z"/></svg>"#.as_bytes())
});
pub fn icon_book() -> Handle { ICON_BOOK.clone() }

static ICON_INFO: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/></svg>"#.as_bytes())
});
pub fn icon_info() -> Handle { ICON_INFO.clone() }

static ICON_ROUTER: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="8" x="2" y="14" rx="2"/><path d="M6.01 18H6"/><path d="M10.01 18H10"/><path d="M14.01 18H14"/><path d="M2 14l3-3"/><path d="M22 14l-3-3"/><path d="M12 14V7"/><path d="M9 10c1.5-1 4.5-1 6 0"/></svg>"#.as_bytes())
});
pub fn icon_router() -> Handle { ICON_ROUTER.clone() }

static ICON_CHECK: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>"#.as_bytes())
});
pub fn icon_check() -> Handle { ICON_CHECK.clone() }

static ICON_APPS: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2v20"/><path d="m2 12 20-3"/><path d="m22 12-20-3"/></svg>"#.as_bytes())
});
pub fn icon_apps() -> Handle { ICON_APPS.clone() }

static ICON_MUSIC: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>"#.as_bytes())
});
pub fn icon_music() -> Handle { ICON_MUSIC.clone() }

static ICON_TRASH: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 6h18"/><path d="M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6"/><path d="M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"/><line x1="10" x2="10" y1="11" y2="17"/><line x1="14" x2="14" y1="11" y2="17"/></svg>"#.as_bytes())
});
pub fn icon_trash() -> Handle { ICON_TRASH.clone() }

static ICON_DOWNLOAD: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" x2="12" y1="15" y2="3"/></svg>"#.as_bytes())
});
pub fn icon_download() -> Handle { ICON_DOWNLOAD.clone() }

static ICON_VIDEO: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="18" height="18" x="3" y="3" rx="2" ry="2"/><path d="m7 8 10 4-10 4V8z"/></svg>"#.as_bytes())
});
pub fn icon_video() -> Handle { ICON_VIDEO.clone() }

static ICON_LAPTOP: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="18" height="12" x="3" y="4" rx="2" ry="2"/><line x1="2" x2="22" y1="20" y2="20"/></svg>"#.as_bytes())
});
pub fn icon_laptop() -> Handle { ICON_LAPTOP.clone() }

static ICON_HEADPHONES: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 14h3a2 2 0 0 1 2 2v3a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-7a9 9 0 0 1 18 0v7a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3"/></svg>"#.as_bytes())
});
pub fn icon_headphones() -> Handle { ICON_HEADPHONES.clone() }

static ICON_BATTERY_LOW: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="16" height="10" x="2" y="7" rx="2" ry="2"/><line x1="22" x2="22" y1="11" y2="13"/><line x1="6" x2="6" y1="11" y2="13"/></svg>"#.as_bytes())
});
pub fn icon_battery_low() -> Handle { ICON_BATTERY_LOW.clone() }

static ICON_BATTERY_FULL: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="16" height="10" x="2" y="7" rx="2" ry="2"/><line x1="22" x2="22" y1="11" y2="13"/><line x1="6" x2="14" y1="11" y2="13"/></svg>"#.as_bytes())
});
pub fn icon_battery_full() -> Handle { ICON_BATTERY_FULL.clone() }

static ICON_SIGNAL: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 20h.01"/><path d="M7 20v-4"/><path d="M12 20v-8"/><path d="M17 20V8"/><path d="M22 20V4"/></svg>"#.as_bytes())
});
pub fn icon_signal() -> Handle { ICON_SIGNAL.clone() }

static ICON_SUN: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg>"#.as_bytes())
});
pub fn icon_sun() -> Handle { ICON_SUN.clone() }

static ICON_HARD_DRIVE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="8" x="2" y="6" rx="2"/><rect width="20" height="8" x="2" y="14" rx="2"/><line x1="6" x2="6.01" y1="10" y2="10"/><line x1="6" x2="6.01" y1="18" y2="18"/></svg>"#.as_bytes())
});
pub fn icon_hard_drive() -> Handle { ICON_HARD_DRIVE.clone() }

static ICON_CLOCK: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>"#.as_bytes())
});
pub fn icon_clock() -> Handle { ICON_CLOCK.clone() }

static ICON_ALERT_TRIANGLE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3Z"/><line x1="12" x2="12" y1="9" y2="13"/><line x1="12" x2="12.01" y1="17" y2="17"/></svg>"#.as_bytes())
});
pub fn icon_alert_triangle() -> Handle { ICON_ALERT_TRIANGLE.clone() }

static ICON_TERMINAL: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 17 10 11 4 5"/><line x1="12" x2="20" y1="19" y2="19"/></svg>"#.as_bytes())
});
pub fn icon_terminal() -> Handle { ICON_TERMINAL.clone() }

static ICON_KEY: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m21 2-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.778-7.778Zm0 0L15.5 7.5m0 0 3 3L22 7l-3-3L15.5 7.5Z"/></svg>"#.as_bytes())
});
pub fn icon_key() -> Handle { ICON_KEY.clone() }

static ICON_SERVER: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="8" x="2" y="2" rx="2" ry="2"/><rect width="20" height="8" x="2" y="14" rx="2" ry="2"/><line x1="6" x2="6.01" y1="6" y2="6"/><line x1="6" x2="6.01" y1="18" y2="18"/></svg>"#.as_bytes())
});
pub fn icon_server() -> Handle { ICON_SERVER.clone() }

static ICON_ACTIVITY: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>"#.as_bytes())
});
pub fn icon_activity() -> Handle { ICON_ACTIVITY.clone() }

static ICON_BRAIN: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9.5 2A2.5 2.5 0 0 1 12 4.5v15a2.5 2.5 0 0 1-4.96.44 2.5 2.5 0 0 1-2.96-3.08 3 3 0 0 1-.34-5.58 2.5 2.5 0 0 1 1.32-4.24 2.5 2.5 0 0 1 4.44-2.04Z"/><path d="M14.5 2A2.5 2.5 0 0 0 12 4.5v15a2.5 2.5 0 0 0 4.96.44 2.5 2.5 0 0 0 2.96-3.08 3 3 0 0 0 .34-5.58 2.5 2.5 0 0 0-1.32-4.24 2.5 2.5 0 0 0-4.44-2.04Z"/></svg>"#.as_bytes())
});
pub fn icon_brain() -> Handle { ICON_BRAIN.clone() }

static ICON_COFFEE: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 8h1a4 4 0 1 1 0 8h-1"/><path d="M3 8h14v9a4 4 0 0 1-4 4H7a4 4 0 0 1-4-4Z"/><line x1="6" x2="6" y1="2" y2="4"/><line x1="10" x2="10" y1="2" y2="4"/><line x1="14" x2="14" y1="2" y2="4"/></svg>"#.as_bytes())
});
pub fn icon_coffee() -> Handle { ICON_COFFEE.clone() }

static ICON_WAYBAR: Lazy<Handle> = Lazy::new(|| {
    Handle::from_memory(r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect width="20" height="4" x="2" y="2" rx="1"/><rect width="20" height="4" x="2" y="18" rx="1"/><path d="M6 10h12"/><path d="M6 14h12"/></svg>"#.as_bytes())
});
pub fn icon_waybar() -> Handle { ICON_WAYBAR.clone() }
