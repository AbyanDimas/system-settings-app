use std::process::Command;
use std::fs;
use crate::error::AppError;

#[derive(Debug, Clone)]
pub struct MakoConfig {
    pub background_color: String,
    pub text_color: String,
    pub border_color: String,
    pub border_radius: u32,
    pub border_size: u32,
    pub padding: u32,
    pub width: u32,
    pub height: u32,
    pub font: String,
}

impl Default for MakoConfig {
    fn default() -> Self {
        Self {
            background_color: "#0f0f0f40".to_string(),
            text_color: "#cdd6f4".to_string(),
            border_color: "#89b4fa".to_string(),
            border_radius: 18,
            border_size: 1,
            padding: 24,
            width: 300,
            height: 100,
            font: "Inter 10".to_string(),
        }
    }
}

pub fn get_mako_config() -> MakoConfig {
    let home = std::env::var("HOME").unwrap_or_default();
    let config_path = format!("{}/.config/mako/config", home);
    
    let mut config = MakoConfig::default();
    
    if let Ok(content) = fs::read_to_string(config_path) {
        for line in content.lines() {
            let parts: Vec<&str> = line.split('=').map(|s| s.trim()).collect();
            if parts.len() == 2 {
                match parts[0] {
                    "background-color" => config.background_color = parts[1].to_string(),
                    "text-color" => config.text_color = parts[1].to_string(),
                    "border-color" => config.border_color = parts[1].to_string(),
                    "border-radius" => config.border_radius = parts[1].parse().unwrap_or(config.border_radius),
                    "border-size" => config.border_size = parts[1].parse().unwrap_or(config.border_size),
                    "padding" => config.padding = parts[1].parse().unwrap_or(config.padding),
                    "width" => config.width = parts[1].parse().unwrap_or(config.width),
                    "height" => config.height = parts[1].parse().unwrap_or(config.height),
                    "font" => config.font = parts[1].to_string(),
                    _ => {}
                }
            }
        }
    }
    
    config
}

pub fn save_mako_config(config: &MakoConfig) -> Result<(), AppError> {
    let home = std::env::var("HOME").unwrap_or_default();
    let config_path = format!("{}/.config/mako/config", home);
    
    // We try to preserve includes and other things by reading first
    let mut lines: Vec<String> = Vec::new();
    let mut keys_found = std::collections::HashSet::new();
    
    if let Ok(content) = fs::read_to_string(&config_path) {
        for line in content.lines() {
            let parts: Vec<&str> = line.split('=').map(|s| s.trim()).collect();
            if parts.len() == 2 {
                match parts[0] {
                    "background-color" => { lines.push(format!("background-color={}", config.background_color)); keys_found.insert("background-color"); }
                    "text-color" => { lines.push(format!("text-color={}", config.text_color)); keys_found.insert("text-color"); }
                    "border-color" => { lines.push(format!("border-color={}", config.border_color)); keys_found.insert("border-color"); }
                    "border-radius" => { lines.push(format!("border-radius={}", config.border_radius)); keys_found.insert("border-radius"); }
                    "border-size" => { lines.push(format!("border-size={}", config.border_size)); keys_found.insert("border-size"); }
                    "padding" => { lines.push(format!("padding={}", config.padding)); keys_found.insert("padding"); }
                    "width" => { lines.push(format!("width={}", config.width)); keys_found.insert("width"); }
                    "height" => { lines.push(format!("height={}", config.height)); keys_found.insert("height"); }
                    "font" => { lines.push(format!("font={}", config.font)); keys_found.insert("font"); }
                    _ => lines.push(line.to_string()),
                }
            } else {
                lines.push(line.to_string());
            }
        }
    }
    
    // Add missing keys
    if !keys_found.contains("background-color") { lines.push(format!("background-color={}", config.background_color)); }
    if !keys_found.contains("text-color") { lines.push(format!("text-color={}", config.text_color)); }
    if !keys_found.contains("border-color") { lines.push(format!("border-color={}", config.border_color)); }
    if !keys_found.contains("border-radius") { lines.push(format!("border-radius={}", config.border_radius)); }
    if !keys_found.contains("border-size") { lines.push(format!("border-size={}", config.border_size)); }
    if !keys_found.contains("padding") { lines.push(format!("padding={}", config.padding)); }
    if !keys_found.contains("width") { lines.push(format!("width={}", config.width)); }
    if !keys_found.contains("height") { lines.push(format!("height={}", config.height)); }
    if !keys_found.contains("font") { lines.push(format!("font={}", config.font)); }

    fs::write(config_path, lines.join("\n")).map_err(|e| AppError::Unknown(e.to_string()))?;
    
    // Reload mako
    let _ = Command::new("makoctl").arg("reload").spawn();
    
    Ok(())
}

#[derive(Debug, Clone)]
pub struct FocusState {
    pub active: bool,
    pub mode: String, // manual, timer
    pub tag: String,
    pub remaining_secs: Option<u64>,
}

pub fn get_focus_state() -> FocusState {
    let home = std::env::var("HOME").unwrap_or_default();
    let state_file = format!("{}/.cache/focus_mode", home);
    
    let active = Command::new("makoctl")
        .arg("mode")
        .output()
        .map(|o| String::from_utf8_lossy(&o.stdout).contains("do-not-disturb"))
        .unwrap_or(false);
        
    let mut state = FocusState {
        active,
        mode: "manual".to_string(),
        tag: "".to_string(),
        remaining_secs: None,
    };
    
    if active && fs::metadata(&state_file).is_ok() {
        if let Ok(content) = fs::read_to_string(&state_file) {
            let parts: Vec<&str> = content.trim().split_whitespace().collect();
            if !parts.is_empty() {
                state.mode = parts[0].to_string();
                if state.mode == "timer" && parts.len() >= 3 {
                    let start: u64 = parts[1].parse().unwrap_or(0);
                    let duration: u64 = parts[2].parse().unwrap_or(0);
                    let now = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap().as_secs();
                    let elapsed = now - start;
                    if elapsed < duration {
                        state.remaining_secs = Some(duration - elapsed);
                    }
                    if parts.len() >= 4 {
                        state.tag = parts[3..].join(" ");
                    }
                } else if parts.len() >= 2 {
                    state.tag = parts[1..].join(" ");
                }
            }
        }
    }
    
    state
}

pub fn set_focus_mode(enabled: bool, tag: Option<&str>) {
    let cmd = if enabled { "on" } else { "off" };
    let mut command = Command::new("sh");
    command.arg("-c");
    if let Some(t) = tag {
        command.arg(format!("~/.config/waybar/scripts/focus-mode.sh {} '{}'", cmd, t));
    } else {
        command.arg(format!("~/.config/waybar/scripts/focus-mode.sh {}", cmd));
    }
    let _ = command.spawn();
}

pub fn set_focus_timer(minutes: u32, tag: Option<&str>) {
    let mut command = Command::new("sh");
    command.arg("-c");
    if let Some(t) = tag {
        command.arg(format!("~/.config/waybar/scripts/focus-mode.sh timer {} '{}'", minutes, t));
    } else {
        command.arg(format!("~/.config/waybar/scripts/focus-mode.sh timer {}", minutes));
    }
    let _ = command.spawn();
}

pub fn send_test_notification() {
    let _ = Command::new("notify-send")
        .args(["-i", "notifications-app", "Test Notification", "This is a test notification from System Settings"])
        .spawn();
}
