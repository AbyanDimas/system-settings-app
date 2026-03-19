use crate::error::AppError;
use hyprland::keyword::Keyword;
use std::process::Command;
use serde::Deserialize;
use serde_json::Value;
use std::fs;

#[derive(Debug, Clone)]
pub struct Keybind {
    pub key: String,
    pub description: String,
    pub dispatcher: String,
    pub arg: String,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AppMonitor {
    pub id: i32,
    pub name: String,
    pub description: String,
    pub make: String,
    pub model: String,
    pub width: i32,
    pub height: i32,
    pub refresh_rate: f32,
    pub scale: f32,
    pub transform: i32,
    pub available_modes: Vec<String>,
}

pub fn get_monitors() -> Result<Vec<AppMonitor>, AppError> {
    let output = Command::new("hyprctl")
        .args(["monitors", "all", "-j"])
        .output()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;

    let s = String::from_utf8_lossy(&output.stdout);
    let monitors: Vec<AppMonitor> = serde_json::from_str(&s).map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(monitors)
}

#[derive(Debug, Clone, Default)]
pub struct InputConfig {
    pub mouse_sensitivity: f32,
    pub touchpad_sensitivity: f32,
    pub touchpad_natural_scroll: bool,
    pub touchpad_tap_to_click: bool,
    pub touchpad_scroll_factor: f32,
    pub gesture_workspace_swipe: bool,
    pub drag_3fg: bool,
    pub numlock_by_default: bool,
}

pub fn get_input_config() -> Result<InputConfig, AppError> {
    let mut config = InputConfig {
        mouse_sensitivity: 0.0,
        touchpad_sensitivity: 0.0,
        touchpad_natural_scroll: true,
        touchpad_tap_to_click: true,
        touchpad_scroll_factor: 0.4,
        gesture_workspace_swipe: true,
        drag_3fg: false,
        numlock_by_default: true,
    };

    let home = std::env::var("HOME").map_err(|_| AppError::Unknown("HOME not set".into()))?;
    let input_path = std::path::PathBuf::from(&home).join(".config/hypr/input.conf");
    let gestures_path = std::path::PathBuf::from(&home).join(".config/hypr/gestures.conf");

    if input_path.exists() {
        let content = std::fs::read_to_string(&input_path).map_err(|e| AppError::Unknown(e.to_string()))?;
        
        // Simple line-by-line parsing for basic values
        for line in content.lines() {
            let line = line.trim();
            if line.starts_with("numlock_by_default") {
                if let Some(val) = line.split('=').nth(1) {
                    config.numlock_by_default = val.trim() == "true";
                }
            }
        }

        // Target device-specific sensitivities if they exist
        let mouse_name = "syna32c5:00-06cb:cd50-mouse";
        let touchpad_name = "syna32c5:00-06cb:cd50-touchpad";

        if let Ok(output) = Command::new("hyprctl").args(["getoption", &format!("device:{}:sensitivity", mouse_name), "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(float) = val["float"].as_f64() { config.mouse_sensitivity = float as f32; }
             }
        }
        
        if let Ok(output) = Command::new("hyprctl").args(["getoption", &format!("device:{}:sensitivity", touchpad_name), "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(float) = val["float"].as_f64() { config.touchpad_sensitivity = float as f32; }
             }
        }

        // Touchpad specific standard options
        if let Ok(output) = Command::new("hyprctl").args(["getoption", "input:touchpad:natural_scroll", "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(int) = val["int"].as_i64() { config.touchpad_natural_scroll = int == 1; }
             }
        }
        if let Ok(output) = Command::new("hyprctl").args(["getoption", "input:touchpad:tap-to-click", "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(int) = val["int"].as_i64() { config.touchpad_tap_to_click = int == 1; }
             }
        }
        if let Ok(output) = Command::new("hyprctl").args(["getoption", "input:touchpad:scroll_factor", "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(float) = val["float"].as_f64() { config.touchpad_scroll_factor = float as f32; }
             }
        }
    }

    if gestures_path.exists() {
        if let Ok(output) = Command::new("hyprctl").args(["getoption", "gestures:workspace_swipe", "-j"]).output() {
             if let Ok(val) = serde_json::from_slice::<Value>(&output.stdout) {
                if let Some(int) = val["int"].as_i64() { config.gesture_workspace_swipe = int == 1; }
             }
        }
    }

    Ok(config)
}

pub fn save_input_config(config: &InputConfig) -> Result<(), AppError> {
    let home = std::env::var("HOME").map_err(|_| AppError::Unknown("HOME not set".into()))?;
    let input_path = format!("{}/.config/hypr/input.conf", home);
    let gestures_path = format!("{}/.config/hypr/gestures.conf", home);

    let mouse_name = "syna32c5:00-06cb:cd50-mouse";
    let touchpad_name = "syna32c5:00-06cb:cd50-touchpad";

    let _ = Command::new("hyprctl").args(["keyword", &format!("device:{}:sensitivity", mouse_name), &config.mouse_sensitivity.to_string()]).status();
    let _ = Command::new("hyprctl").args(["keyword", &format!("device:{}:sensitivity", touchpad_name), &config.touchpad_sensitivity.to_string()]).status();
    let _ = Command::new("hyprctl").args(["keyword", "input:touchpad:natural_scroll", if config.touchpad_natural_scroll { "true" } else { "false" }]).status();
    let _ = Command::new("hyprctl").args(["keyword", "input:touchpad:tap-to-click", if config.touchpad_tap_to_click { "true" } else { "false" }]).status();
    let _ = Command::new("hyprctl").args(["keyword", "input:touchpad:scroll_factor", &config.touchpad_scroll_factor.to_string()]).status();
    let _ = Command::new("hyprctl").args(["keyword", "gestures:workspace_swipe", if config.gesture_workspace_swipe { "true" } else { "false" }]).status();

    let input_content = fs::read_to_string(&input_path).unwrap_or_default();
    let lines: Vec<String> = input_content.lines().map(|s| s.to_string()).collect();
    
    let mut new_lines = Vec::new();
    let mut skipping = false;
    for line in lines {
        let is_closing_brace = line.trim() == "}";
        if line.contains(&format!("device:{}", mouse_name)) || line.contains(&format!("device:{}", touchpad_name)) {
            skipping = true;
        }
        if !skipping {
            new_lines.push(line);
        }
        if skipping && is_closing_brace {
            skipping = false;
        }
    }

    new_lines.push(format!("device:{} {{", mouse_name));
    new_lines.push(format!("    sensitivity = {:.2}", config.mouse_sensitivity));
    new_lines.push("}".to_string());
    
    new_lines.push(format!("device:{} {{", touchpad_name));
    new_lines.push(format!("    sensitivity = {:.2}", config.touchpad_sensitivity));
    new_lines.push("}".to_string());

    let _ = fs::write(&input_path, new_lines.join("\n"));

    let _ = Command::new("sed").args(["-i", &format!("s/natural_scroll = .*/natural_scroll = {}/", config.touchpad_natural_scroll), &input_path]).status();
    let _ = Command::new("sed").args(["-i", &format!("s/tap-to-click = .*/tap-to-click = {}/", config.touchpad_tap_to_click), &input_path]).status();
    let _ = Command::new("sed").args(["-i", &format!("s/scroll_factor = .*/scroll_factor = {:.2}/", config.touchpad_scroll_factor), &input_path]).status();
    let _ = Command::new("sed").args(["-i", &format!("s/numlock_by_default = .*/numlock_by_default = {}/", config.numlock_by_default), &input_path]).status();
    let _ = Command::new("sed").args(["-i", &format!("s/workspace_swipe = .*/workspace_swipe = {}/", config.gesture_workspace_swipe), &gestures_path]).status();

    Ok(())
}

pub fn set_resolution(monitor_name: &str, res: &str, refresh: &str) -> Result<(), AppError> {
    let arg = format!("{},{}@{},auto,1", monitor_name, res, refresh);
    Keyword::set("monitor", arg.as_str()).map_err(|e| AppError::Hyprland(e.to_string()))
}

pub fn set_scale(monitor_name: &str, scale: f32) -> Result<(), AppError> {
    let arg = format!("{},preferred,auto,{}", monitor_name, scale);
    Keyword::set("monitor", arg.as_str()).map_err(|e| AppError::Hyprland(e.to_string()))
}

pub fn set_scale_auto(monitor_name: &str) -> Result<(), AppError> {
    let arg = format!("{},preferred,auto,auto", monitor_name);
    Keyword::set("monitor", arg.as_str()).map_err(|e| AppError::Hyprland(e.to_string()))
}

pub fn set_transform(monitor_name: &str, transform: i32) -> Result<(), AppError> {
    let arg = format!("{},transform,{}", monitor_name, transform);
    Keyword::set("monitor", arg.as_str()).map_err(|e| AppError::Hyprland(e.to_string()))
}

pub fn set_keyboard_brightness(percent: u32) -> Result<(), AppError> {
    Command::new("brightnessctl")
        .args(["-d", "*kbd*", "set", &format!("{}%", percent)])
        .status()
        .map_err(|e| AppError::Unknown(e.to_string()))?;
    Ok(())
}

pub fn set_swap_ctrl_alt(enabled: bool) -> Result<(), AppError> {
    let _val = if enabled { "1" } else { "0" };
    Command::new("hyprctl")
        .args(["keyword", "input:kb_options", if enabled { "ctrl:swap_lalt_lctl" } else { "" }])
        .status()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(())
}

pub fn set_touchpad_natural_scroll(enabled: bool) -> Result<(), AppError> {
    let val = if enabled { "true" } else { "false" };
    Command::new("hyprctl")
        .args(["keyword", "input:touchpad:natural_scroll", val])
        .status()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(())
}

pub fn set_touchpad_tap_to_click(enabled: bool) -> Result<(), AppError> {
    let val = if enabled { "true" } else { "false" };
    Command::new("hyprctl")
        .args(["keyword", "input:touchpad:tap-to-click", val])
        .status()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(())
}

pub fn set_touchpad_scroll_factor(factor: f32) -> Result<(), AppError> {
    Command::new("hyprctl")
        .args(["keyword", "input:touchpad:scroll_factor", &factor.to_string()])
        .status()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(())
}

pub fn set_gesture_workspace_swipe(enabled: bool) -> Result<(), AppError> {
    let val = if enabled { "true" } else { "false" };
    Command::new("hyprctl")
        .args(["keyword", "gestures:workspace_swipe", val])
        .status()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;
    Ok(())
}

pub fn get_keybinds() -> Result<Vec<Keybind>, AppError> {
    let output = Command::new("hyprctl")
        .arg("binds")
        .output()
        .map_err(|e| AppError::Hyprland(e.to_string()))?;

    let s = String::from_utf8_lossy(&output.stdout);
    let mut binds = Vec::new();
    let mut current_bind = None;

    for line in s.lines() {
        let line = line.trim();
        if line.starts_with("bind") {
            if let Some(b) = current_bind.take() {
                binds.push(b);
            }
            current_bind = Some(Keybind {
                key: String::new(),
                description: String::new(),
                dispatcher: String::new(),
                arg: String::new(),
            });
        } else if let Some(ref mut b) = current_bind {
            if let Some(val) = line.strip_prefix("key: ") {
                b.key = val.to_string();
            } else if let Some(val) = line.strip_prefix("description: ") {
                b.description = val.to_string();
            } else if let Some(val) = line.strip_prefix("dispatcher: ") {
                b.dispatcher = val.to_string();
            } else if let Some(val) = line.strip_prefix("arg: ") {
                b.arg = val.to_string();
            }
        }
    }
    
    if let Some(b) = current_bind {
        binds.push(b);
    }

    Ok(binds)
}

#[derive(Debug, Clone)]
pub struct StartupApp {
    pub command: String,
}

pub fn get_startup_apps() -> Result<Vec<StartupApp>, AppError> {
    let mut apps = Vec::new();
    let home = std::env::var("HOME").map_err(|_| AppError::Unknown("HOME not set".into()))?;
    let config_path = std::path::PathBuf::from(&home).join(".config/hypr/hyprland.conf");

    parse_config_for_apps(&config_path, &mut apps, &home)?;

    Ok(apps)
}

fn parse_config_for_apps(path: &std::path::Path, apps: &mut Vec<StartupApp>, home: &str) -> Result<(), AppError> {
    if !path.exists() {
        return Ok(());
    }

    let content = std::fs::read_to_string(path).map_err(|e| AppError::Unknown(e.to_string()))?;

    for line in content.lines() {
        let line = line.trim();
        if line.starts_with("exec-once") {
            if let Some(cmd) = line.splitn(2, '=').nth(1) {
                let cmd = cmd.trim().to_string();
                if !cmd.is_empty() {
                    apps.push(StartupApp { command: cmd });
                }
            }
        } else if line.starts_with("source") {
            if let Some(source_path_raw) = line.splitn(2, '=').nth(1) {
                let mut source_path_str = source_path_raw.trim().to_string();
                if source_path_str.starts_with('~') {
                    source_path_str = source_path_str.replace('~', home);
                }
                let source_path = std::path::PathBuf::from(source_path_str);
                let _ = parse_config_for_apps(&source_path, apps, home);
            }
        }
    }

    Ok(())
}
