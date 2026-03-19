use crate::error::AppError;
use std::process::Command;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fs;
use std::path::PathBuf;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WaybarConfig {
    pub layer: String,
    pub position: String,
    pub height: Option<u32>,
    pub spacing: Option<u32>,
    pub margin: Option<String>,
    #[serde(rename = "modules-left")]
    pub modules_left: Vec<String>,
    #[serde(rename = "modules-center")]
    pub modules_center: Vec<String>,
    #[serde(rename = "modules-right")]
    pub modules_right: Vec<String>,
    #[serde(flatten)]
    pub other: std::collections::HashMap<String, Value>,
}

pub struct WaybarService;

impl WaybarService {
    fn get_config_path() -> PathBuf {
        let home = std::env::var("HOME").unwrap_or_default();
        PathBuf::from(home).join(".config/waybar/config.jsonc")
    }

    pub fn load_config() -> Result<WaybarConfig, AppError> {
        let path = Self::get_config_path();
        let content = fs::read_to_string(&path)
            .map_err(|e| AppError::Config(format!("Failed to read waybar config: {}", e)))?;
        
        // Waybar uses JSONC (JSON with comments). Simple hack: remove lines starting with //
        // A better approach would be a real JSONC parser if needed.
        let cleaned_content: String = content.lines()
            .filter(|line| !line.trim().starts_with("//"))
            .collect::<Vec<_>>()
            .join("\n");

        serde_json::from_str(&cleaned_content)
            .map_err(|e| AppError::Config(format!("Failed to parse waybar config: {}", e)))
    }

    pub fn save_config(config: &WaybarConfig) -> Result<(), AppError> {
        let path = Self::get_config_path();
        let content = serde_json::to_string_pretty(config)
            .map_err(|e| AppError::Config(format!("Failed to serialize waybar config: {}", e)))?;
        
        fs::write(path, content)
            .map_err(|e| AppError::Config(format!("Failed to write waybar config: {}", e)))?;
        
        Self::reload_config()
    }

    pub fn restart() -> Result<(), AppError> {
        // Fully restart waybar
        let _ = Command::new("pkill").arg("waybar").status();
        Command::new("waybar").spawn().map_err(|e| AppError::System(e.to_string()))?;
        Ok(())
    }

    pub fn is_running() -> bool {
        Command::new("pgrep")
            .arg("waybar")
            .status()
            .map(|s| s.success())
            .unwrap_or(false)
    }

    pub fn reload_config() -> Result<(), AppError> {
        Command::new("pkill")
            .arg("-USR2")
            .arg("waybar")
            .status()
            .map_err(|e| AppError::System(e.to_string()))?;
        Ok(())
    }
}
