use crate::error::AppError;
use std::process::Command;

#[derive(Debug, Clone, PartialEq)]
pub struct AudioDevice {
    pub id: String, // Use Name as ID for reliability with pactl
    pub description: String,
    pub volume: u32,
    pub is_muted: bool,
    pub is_default: bool,
    pub device_type: DeviceType,
}

impl std::fmt::Display for AudioDevice {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.description)
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum DeviceType {
    Speaker,
    Headphones,
    HDMI,
    Microphone,
    Other,
}

#[derive(Debug, Clone, PartialEq)]
pub struct AudioProfile {
    pub name: String,
    pub description: String,
    pub is_active: bool,
}

impl std::fmt::Display for AudioProfile {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.description)
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct AudioCard {
    pub id: String,
    pub name: String,
    pub description: String,
    pub profiles: Vec<AudioProfile>,
    pub active_profile: String,
}

#[derive(Clone)]
pub struct AudioService {}

impl AudioService {
    pub fn new() -> Result<Self, AppError> {
        Ok(Self {})
    }

    pub fn get_sinks(&self) -> Result<Vec<AudioDevice>, AppError> {
        self.parse_pactl_list("sinks")
    }

    pub fn get_sources(&self) -> Result<Vec<AudioDevice>, AppError> {
        // Filter out "monitor" sources to only show real microphones
        let sources = self.parse_pactl_list("sources")?;
        Ok(sources.into_iter().filter(|s| !s.id.contains(".monitor")).collect())
    }

    pub fn get_cards(&self) -> Result<Vec<AudioCard>, AppError> {
        let output = Command::new("pactl")
            .args(["list", "cards"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = String::from_utf8_lossy(&output.stdout);
        let mut cards = Vec::new();
        let mut current_card: Option<AudioCard> = None;
        let mut in_profiles = false;

        for line in s.lines() {
            if line.starts_with("Card #") {
                if let Some(c) = current_card.take() {
                    cards.push(c);
                }
                current_card = Some(AudioCard {
                    id: String::new(),
                    name: String::new(),
                    description: String::new(),
                    profiles: Vec::new(),
                    active_profile: String::new(),
                });
                in_profiles = false;
                continue;
            }

            let trimmed = line.trim();
            if let Some(card) = current_card.as_mut() {
                if trimmed.starts_with("Name: ") {
                    card.id = trimmed["Name: ".len()..].to_string();
                    card.name = card.id.clone();
                } else if trimmed.starts_with("Description: ") {
                    card.description = trimmed["Description: ".len()..].to_string();
                } else if trimmed == "Profiles:" {
                    in_profiles = true;
                } else if trimmed.starts_with("Active Profile: ") {
                    in_profiles = false;
                    let active = trimmed["Active Profile: ".len()..].to_string();
                    card.active_profile = active.clone();
                    for p in &mut card.profiles {
                        if p.name == active {
                            p.is_active = true;
                        }
                    }
                } else if in_profiles && trimmed.contains(": ") {
                    // Profile format: "hifi: HiFi (sinks: 2, sources: 1, ...)"
                    let parts: Vec<&str> = trimmed.splitn(2, ':').collect();
                    if parts.len() == 2 {
                        card.profiles.push(AudioProfile {
                            name: parts[0].to_string(),
                            description: parts[1].split('(').next().unwrap_or(parts[1]).trim().to_string(),
                            is_active: false,
                        });
                    }
                }
            }
        }
        if let Some(c) = current_card {
            cards.push(c);
        }
        Ok(cards)
    }

    fn parse_pactl_list(&self, kind: &str) -> Result<Vec<AudioDevice>, AppError> {
        let output = Command::new("pactl")
            .args(["list", kind])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = String::from_utf8_lossy(&output.stdout);
        let mut devices = Vec::new();
        let mut current_dev: Option<AudioDevice> = None;

        let default_cmd = if kind == "sinks" { "get-default-sink" } else { "get-default-source" };
        let default_output = Command::new("pactl").arg(default_cmd).output().ok();
        let default_name = default_output.map(|o| String::from_utf8_lossy(&o.stdout).trim().to_string()).unwrap_or_default();

        for line in s.lines() {
            if line.starts_with("Sink #") || line.starts_with("Source #") {
                if let Some(d) = current_dev.take() {
                    devices.push(d);
                }
                current_dev = Some(AudioDevice {
                    id: String::new(),
                    description: String::new(),
                    volume: 0,
                    is_muted: false,
                    is_default: false,
                    device_type: DeviceType::Other,
                });
                continue;
            }

            let trimmed = line.trim();
            if let Some(dev) = current_dev.as_mut() {
                if trimmed.starts_with("Name: ") {
                    dev.id = trimmed["Name: ".len()..].to_string();
                    dev.is_default = dev.id == default_name;
                } else if trimmed.starts_with("Description: ") {
                    dev.description = trimmed["Description: ".len()..].to_string();
                    dev.device_type = if dev.description.to_lowercase().contains("headphone") {
                        DeviceType::Headphones
                    } else if dev.description.to_lowercase().contains("hdmi") || dev.description.to_lowercase().contains("displayport") {
                        DeviceType::HDMI
                    } else if dev.description.to_lowercase().contains("mic") {
                        DeviceType::Microphone
                    } else if dev.description.to_lowercase().contains("speaker") {
                        DeviceType::Speaker
                    } else {
                        DeviceType::Other
                    };
                } else if trimmed.starts_with("Mute: ") {
                    dev.is_muted = trimmed.contains("yes");
                } else if trimmed.starts_with("Volume: ") {
                    // Extract percentage: "... /  39% / ..."
                    if let Some(perc_idx) = trimmed.find('%') {
                        let start = trimmed[..perc_idx].rfind(' ').unwrap_or(0);
                        if let Ok(vol) = trimmed[start..perc_idx].trim().parse::<u32>() {
                            dev.volume = vol;
                        }
                    }
                }
            }
        }
        if let Some(d) = current_dev {
            devices.push(d);
        }
        Ok(devices)
    }

    pub fn set_default(&self, name: &str, is_sink: bool) -> Result<(), AppError> {
        let cmd = if is_sink { "set-default-sink" } else { "set-default-source" };
        Command::new("pactl")
            .args([cmd, name])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn set_volume(&self, name: &str, percent: u32) -> Result<(), AppError> {
        Command::new("pactl")
            .args(["set-sink-volume", name, &format!("{}%", percent)])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn set_source_volume(&self, name: &str, percent: u32) -> Result<(), AppError> {
        Command::new("pactl")
            .args(["set-source-volume", name, &format!("{}%", percent)])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn toggle_mute(&self, name: &str, is_sink: bool) -> Result<(), AppError> {
        let cmd = if is_sink { "set-sink-mute" } else { "set-source-mute" };
        Command::new("pactl")
            .args([cmd, name, "toggle"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn set_card_profile(&self, card_name: &str, profile_name: &str) -> Result<(), AppError> {
        Command::new("pactl")
            .args(["set-card-profile", card_name, profile_name])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }
}
