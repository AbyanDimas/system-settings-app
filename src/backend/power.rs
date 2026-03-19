use crate::error::AppError;
use std::process::Command;
use std::fs;
use std::path::PathBuf;

#[derive(Debug, Clone)]
pub struct BatteryInfo {
    pub percentage: u32,
    pub is_charging: bool,
    pub state: String, // discharging, charging, full
    pub power_now: f32, // W
    pub health: f32, // Capacity %
    pub cycle_count: u32,
    pub technology: String,
    pub energy_now: f32, // Wh
    pub energy_full: f32, // Wh
    pub energy_full_design: f32, // Wh
    pub voltage: f32, // V
    pub voltage_min_design: f32, // V
    pub vendor: String,
    pub model: String,
}

#[derive(Debug, Clone)]
pub struct HypridleConfig {
    pub lock_timeout: u32,
    pub dpms_timeout: u32,
    pub suspend_timeout: u32,
    pub hibernate_timeout: u32,
}

impl Default for HypridleConfig {
    fn default() -> Self {
        Self {
            lock_timeout: 600,
            dpms_timeout: 900,
            suspend_timeout: 1200,
            hibernate_timeout: 7200,
        }
    }
}

pub struct PowerService {}

impl PowerService {
    pub fn new() -> Result<Self, AppError> {
        Ok(Self {})
    }

    pub fn get_hypridle_config(&self) -> HypridleConfig {
        let mut config = HypridleConfig::default();
        let home = std::env::var("HOME").unwrap_or_default();
        let path = PathBuf::from(home).join(".config/hypr/hypridle.conf");
        
        if let Ok(content) = fs::read_to_string(&path) {
            let mut current_timeout = 0;
            
            for line in content.lines() {
                let trimmed = line.trim();
                if trimmed.starts_with("timeout") {
                    if let Some(val_str) = trimmed.split('=').nth(1) {
                        // Extract number before any comments
                        if let Some(num_str) = val_str.split('#').next() {
                            current_timeout = num_str.trim().parse().unwrap_or(0);
                        }
                    }
                } else if trimmed.starts_with("on-timeout") {
                    if trimmed.contains("loginctl lock-session") {
                        config.lock_timeout = current_timeout;
                    } else if trimmed.contains("hyprctl dispatch dpms off") {
                        config.dpms_timeout = current_timeout;
                    } else if trimmed.contains("systemctl suspend-then-hibernate") || trimmed.contains("systemctl suspend") {
                        config.suspend_timeout = current_timeout;
                    } else if trimmed.contains("systemctl hibernate") && !trimmed.contains("suspend-then-hibernate") {
                        config.hibernate_timeout = current_timeout;
                    }
                }
            }
        }
        config
    }

    pub fn set_hypridle_config(&self, config: &HypridleConfig) -> Result<(), AppError> {
        let home = std::env::var("HOME").unwrap_or_default();
        let path = PathBuf::from(home).join(".config/hypr/hypridle.conf");
        
        if let Ok(content) = fs::read_to_string(&path) {
            let mut new_content = String::new();
            let mut current_timeout_line = String::new();
            
            for line in content.lines() {
                let trimmed = line.trim();
                
                if trimmed.starts_with("timeout") {
                    current_timeout_line = line.to_string();
                    // Don't push it yet, we need to see what the on-timeout is
                    continue;
                } else if trimmed.starts_with("on-timeout") {
                    if trimmed.contains("loginctl lock-session") {
                        let replaced = Self::replace_timeout_value(&current_timeout_line, config.lock_timeout);
                        new_content.push_str(&replaced);
                        new_content.push('\n');
                    } else if trimmed.contains("hyprctl dispatch dpms off") {
                        let replaced = Self::replace_timeout_value(&current_timeout_line, config.dpms_timeout);
                        new_content.push_str(&replaced);
                        new_content.push('\n');
                    } else if trimmed.contains("systemctl suspend-then-hibernate") || trimmed.contains("systemctl suspend") {
                        let replaced = Self::replace_timeout_value(&current_timeout_line, config.suspend_timeout);
                        new_content.push_str(&replaced);
                        new_content.push('\n');
                    } else if trimmed.contains("systemctl hibernate") && !trimmed.contains("suspend-then-hibernate") {
                        let replaced = Self::replace_timeout_value(&current_timeout_line, config.hibernate_timeout);
                        new_content.push_str(&replaced);
                        new_content.push('\n');
                    } else {
                        // Some other listener, keep the original timeout
                        new_content.push_str(&current_timeout_line);
                        new_content.push('\n');
                    }
                    new_content.push_str(line);
                    new_content.push('\n');
                    current_timeout_line.clear();
                    continue;
                } else {
                    // If we had a timeout line and it wasn't followed by on-timeout, just push it
                    if !current_timeout_line.is_empty() {
                        new_content.push_str(&current_timeout_line);
                        new_content.push('\n');
                        current_timeout_line.clear();
                    }
                    new_content.push_str(line);
                    new_content.push('\n');
                }
            }
            
            fs::write(&path, new_content).map_err(|e| AppError::Unknown(e.to_string()))?;
            
            // Restart hypridle
            Command::new("sh")
                .args(["-c", "killall hypridle; hyprctl dispatch exec hypridle &"])
                .spawn()
                .ok();
        }
        
        Ok(())
    }
    
    fn replace_timeout_value(line: &str, new_value: u32) -> String {
        // e.g. "    timeout = 600                      # 5min" -> "    timeout = 1200                     # 5min"
        if let Some(idx) = line.find('=') {
            let prefix = &line[..=idx];
            if let Some(comment_idx) = line.find('#') {
                let padding = " ".repeat(line[idx+1..comment_idx].chars().count().saturating_sub(new_value.to_string().len() + 1));
                format!("{} {}{}{}", prefix, new_value, padding, &line[comment_idx..])
            } else {
                format!("{} {}", prefix, new_value)
            }
        } else {
            line.to_string()
        }
    }

    pub fn get_battery_info(&self) -> Result<BatteryInfo, AppError> {
        let output = Command::new("upower")
            .args(["-i", "/org/freedesktop/UPower/devices/battery_BAT0"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = String::from_utf8_lossy(&output.stdout);
        let mut info = BatteryInfo {
            percentage: 0,
            is_charging: false,
            state: "unknown".into(),
            power_now: 0.0,
            health: 0.0,
            cycle_count: 0,
            technology: "unknown".into(),
            energy_now: 0.0,
            energy_full: 0.0,
            energy_full_design: 0.0,
            voltage: 0.0,
            voltage_min_design: 0.0,
            vendor: "unknown".into(),
            model: "unknown".into(),
        };

        for line in s.lines() {
            let line = line.trim();
            if line.starts_with("percentage:") {
                info.percentage = line["percentage:".len()..].trim().trim_end_matches('%').parse().unwrap_or(0);
            } else if line.starts_with("state:") {
                info.state = line["state:".len()..].trim().to_string();
                info.is_charging = info.state == "charging";
            } else if line.starts_with("energy-rate:") {
                info.power_now = line["energy-rate:".len()..].trim().trim_end_matches('W').trim().parse().unwrap_or(0.0);
            } else if line.starts_with("capacity:") {
                info.health = line["capacity:".len()..].trim().trim_end_matches('%').trim().parse().unwrap_or(0.0);
            } else if line.starts_with("charge-cycles:") {
                info.cycle_count = line["charge-cycles:".len()..].trim().parse().unwrap_or(0);
            } else if line.starts_with("technology:") {
                info.technology = line["technology:".len()..].trim().to_string();
            } else if line.starts_with("energy:") {
                info.energy_now = line["energy:".len()..].trim().trim_end_matches("Wh").trim().parse().unwrap_or(0.0);
            } else if line.starts_with("energy-full:") {
                info.energy_full = line["energy-full:".len()..].trim().trim_end_matches("Wh").trim().parse().unwrap_or(0.0);
            } else if line.starts_with("energy-full-design:") {
                info.energy_full_design = line["energy-full-design:".len()..].trim().trim_end_matches("Wh").trim().parse().unwrap_or(0.0);
            } else if line.starts_with("voltage:") {
                info.voltage = line["voltage:".len()..].trim().trim_end_matches('V').trim().parse().unwrap_or(0.0);
            } else if line.starts_with("voltage-min-design:") {
                info.voltage_min_design = line["voltage-min-design:".len()..].trim().trim_end_matches('V').trim().parse().unwrap_or(0.0);
            } else if line.starts_with("vendor:") {
                info.vendor = line["vendor:".len()..].trim().to_string();
            } else if line.starts_with("model:") {
                info.model = line["model:".len()..].trim().to_string();
            }
        }

        Ok(info)
    }

    pub fn get_power_mode(&self) -> Result<String, AppError> {
        let output = Command::new("powerprofilesctl")
            .args(["get"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        Ok(String::from_utf8_lossy(&output.stdout).trim().to_string())
    }

    pub fn set_power_mode(&self, mode: &str) -> Result<(), AppError> {
        Command::new("powerprofilesctl")
            .args(["set", mode])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }
}
