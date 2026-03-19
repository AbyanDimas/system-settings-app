use crate::error::AppError;
use std::process::Command;

#[derive(Clone)]
pub struct NetworkService {}

#[derive(Debug, Clone)]
pub struct WifiNetwork {
    pub ssid: String,
    pub security: String,
    pub is_connected: bool,
    pub is_known: bool,
}

#[derive(Debug, Clone)]
pub struct HotspotDevice {
    pub mac: String,
    pub connected_time: String,
}

#[derive(Debug, Clone)]
pub struct VpnInfo {
    pub name: String,
    pub status: String,
    pub is_running: bool,
    pub icon_path: Option<String>,
}

fn strip_ansi(s: &str) -> String {
    let mut result = String::new();
    let mut skipping = false;
    for c in s.chars() {
        if c == '\x1b' {
            skipping = true;
        } else if skipping {
            if c.is_alphabetic() {
                skipping = false;
            }
        } else {
            result.push(c);
        }
    }
    result
}

impl NetworkService {
    pub fn new() -> Result<Self, AppError> {
        Ok(Self {})
    }

    pub fn get_wifi_device(&self) -> String {
        if let Ok(output) = Command::new("iwctl").args(["device", "list"]).output() {
            let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
            for line in s.lines() {
                let parts: Vec<&str> = line.split_whitespace().collect();
                if parts.len() >= 3 && !line.contains("---") && parts[0] != "Name" && parts[0] != "Devices" {
                    return parts[0].to_string();
                }
            }
        }
        "wlan0".to_string()
    }

    pub fn is_wifi_on(&self) -> Result<bool, AppError> {
        let output = Command::new("iwctl")
            .args(["device", "list"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
        let dev = self.get_wifi_device();
        for line in s.lines() {
            if line.contains(&dev) {
                return Ok(line.contains(" on ") || line.contains(" yes "));
            }
        }
        Ok(false)
    }

    pub fn scan(&self) -> Result<(), AppError> {
        let dev = self.get_wifi_device();
        Command::new("iwctl")
            .args(["station", &dev, "scan"])
            .status()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn get_current_ssid(&self) -> Result<String, AppError> {
        let dev = self.get_wifi_device();
        let output = Command::new("iwctl")
            .args(["station", &dev, "show"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
        for line in s.lines() {
            if line.contains("Connected network") {
                if let Some(pos) = line.find("Connected network") {
                    let ssid_part = &line[pos + "Connected network".len()..].trim();
                    if !ssid_part.is_empty() {
                        return Ok(ssid_part.to_string());
                    }
                }
            }
        }
        Ok("Not Connected".to_string())
    }

    pub fn get_available_networks(&self) -> Result<Vec<WifiNetwork>, AppError> {
        let mut known_ssids = Vec::new();
        if let Ok(output) = Command::new("iwctl").args(["known-networks", "list"]).output() {
            let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
            let mut parsing = false;
            for line in s.lines() {
                if line.contains("---") {
                    parsing = true;
                    continue;
                }
                if parsing && !line.trim().is_empty() {
                    let parts: Vec<&str> = line.trim().split("  ").filter(|s| !s.is_empty()).collect();
                    if !parts.is_empty() {
                        known_ssids.push(parts[0].trim().to_string()); 
                    }
                }
            }
        }

        let dev = self.get_wifi_device();
        let output = Command::new("iwctl")
            .args(["station", &dev, "get-networks"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
        let mut networks = Vec::new();
        let mut parsing = false;
        for line in s.lines() {
            if line.contains("---") {
                parsing = true;
                continue;
            }
            if parsing && !line.trim().is_empty() {
                let is_connected = line.starts_with("  >");
                let line_content = if line.len() > 4 { &line[4..] } else { line };
                let parts: Vec<&str> = line_content.trim().split("  ").filter(|s| !s.trim().is_empty()).collect();
                
                if !parts.is_empty() {
                    let ssid = parts[0].trim().to_string();
                    let security = if parts.len() > 1 { parts[1].trim().to_string() } else { "open".to_string() };
                    let is_known = known_ssids.contains(&ssid);

                    networks.push(WifiNetwork {
                        ssid,
                        security,
                        is_connected,
                        is_known,
                    });
                }
            }
        }
        
        networks.sort_by(|a, b| b.is_connected.cmp(&a.is_connected));
        networks.dedup_by(|a, b| a.ssid == b.ssid);

        Ok(networks)
    }

    pub fn connect_to_network(&self, ssid: &str, password: Option<&str>) -> Result<(), AppError> {
        let mut cmd = Command::new("iwctl");
        if let Some(pass) = password {
            if !pass.is_empty() {
                cmd.args(["--passphrase", pass]);
            }
        }
        let dev = self.get_wifi_device();
        cmd.args(["station", &dev, "connect", ssid]);
        let output = cmd.output().map_err(|e| AppError::Unknown(e.to_string()))?;
        if !output.status.success() {
            return Err(AppError::Unknown(strip_ansi(&String::from_utf8_lossy(&output.stderr))));
        }
        Ok(())
    }

    pub fn forget_network(&self, ssid: &str) -> Result<(), AppError> {
        let output = Command::new("iwctl")
            .args(["known-network", ssid, "forget"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        if !output.status.success() {
            return Err(AppError::Unknown(strip_ansi(&String::from_utf8_lossy(&output.stderr))));
        }
        Ok(())
    }

    pub fn set_wifi_powered(&self, _device: &str, powered: bool) -> Result<(), AppError> {
        let dev = self.get_wifi_device();
        let state = if powered { "on" } else { "off" };
        Command::new("iwctl")
            .args(["adapter", "phy0", "set-property", "Powered", state])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }

    pub fn get_hotspot_devices(&self) -> Result<Vec<HotspotDevice>, AppError> {
        let dev = self.get_wifi_device();
        let output = Command::new("iwctl")
            .args(["ap", &dev, "show"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
        let mut devices = Vec::new();
        let mut parsing_clients = false;
        for line in s.lines() {
            if line.contains("Clients") {
                parsing_clients = true;
                continue;
            }
            if parsing_clients && !line.trim().is_empty() && !line.contains("Address") && !line.contains("---") {
                let parts: Vec<&str> = line.trim().split_whitespace().collect();
                if !parts.is_empty() {
                    let mac = parts[0].to_string();
                    let connected_time = if parts.len() > 1 { parts[1..].join(" ") } else { "Unknown".into() };
                    devices.push(HotspotDevice { mac, connected_time });
                }
            }
        }
        Ok(devices)
    }

    pub fn get_dns_servers(&self) -> Vec<String> {
        if let Ok(content) = std::fs::read_to_string("/etc/resolv.conf") {
            content.lines()
                .filter(|l| l.starts_with("nameserver"))
                .map(|l| l.split_whitespace().nth(1).unwrap_or("").to_string())
                .filter(|s| !s.is_empty())
                .collect()
        } else {
            Vec::new()
        }
    }

    pub fn get_vpn_status(&self) -> Vec<VpnInfo> {
        let mut vpns = Vec::new();
        let tailscale_running = Command::new("tailscale")
            .arg("status")
            .output()
            .map(|o| o.status.success())
            .unwrap_or(false);
        vpns.push(VpnInfo {
            name: "Tailscale".to_string(),
            status: if tailscale_running { "Connected".to_string() } else { "Disconnected".to_string() },
            is_running: tailscale_running,
            icon_path: Some("/home/abyandimas/.local/share/icons/OneUI/scalable/apps/tailscale.svg".to_string()),
        });
        if let Ok(output) = Command::new("ip").args(["link", "show"]).output() {
            let s = String::from_utf8_lossy(&output.stdout);
            if s.contains("wg0") || s.contains("tun0") {
                vpns.push(VpnInfo {
                    name: if s.contains("wg0") { "WireGuard (wg0)" } else { "OpenVPN (tun0)" }.to_string(),
                    status: "Connected".to_string(),
                    is_running: true,
                    icon_path: None,
                });
            }
        }
        vpns
    }

    pub fn set_tailscale_active(&self, active: bool) -> Result<(), AppError> {
        let arg = if active { "up" } else { "down" };
        let status = Command::new("tailscale").arg(arg).status().map_err(|e| AppError::Unknown(e.to_string()))?;
        if status.success() { Ok(()) } else { Err(AppError::Unknown(format!("Failed to set Tailscale {}", arg))) }
    }

    pub fn set_dns(&self, servers: &str) -> Result<(), AppError> {
        let dev = self.get_wifi_device();
        let status = Command::new("sudo").args(["resolvectl", "dns", &dev]).args(servers.split_whitespace()).status().map_err(|e| AppError::Unknown(e.to_string()))?;
        if status.success() {
            let _ = Command::new("sudo").args(["resolvectl", "flush-caches"]).status();
            Ok(())
        } else { Err(AppError::Unknown("Failed to set DNS servers".to_string())) }
    }

    pub fn get_connection_details(&self) -> Result<ConnectionDetails, AppError> {
        let mut details = ConnectionDetails::default();
        let dev = self.get_wifi_device();

        // Get Gateway IP from ip route
        if let Ok(output) = Command::new("ip").args(["route", "show", "default", "dev", &dev]).output() {            let s = String::from_utf8_lossy(&output.stdout);
            // Example: default via 192.168.1.1 proto dhcp src 192.168.1.45 metric 600 
            if let Some(pos) = s.find("via ") {
                let part = &s[pos + 4..];
                if let Some(end) = part.find(' ') {
                    details.router = part[..end].to_string();
                }
            }
        }

        // Get more details from iwctl station show
        if let Ok(output) = Command::new("iwctl").args(["station", &dev, "show"]).output() {
            let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
            for line in s.lines() {
                let line = line.trim();
                if line.contains("AverageRSSI") || line.contains("RSSI") {
                    // Look for numbers like -50
                    let parts: Vec<&str> = line.split_whitespace().collect();
                    for p in parts {
                        if let Ok(val) = p.parse::<i32>() {
                            details.signal_strength = val;
                            break;
                        }
                    }
                } else if line.contains("IPv4 address") {
                    details.ip_address = line.split_whitespace().last().unwrap_or("").to_string();
                } else if line.contains("Security") {
                    details.security = line.split_whitespace().last().unwrap_or("").to_string();
                }
            }
        }

        Ok(details)
    }
}

#[derive(Debug, Clone, Default)]
pub struct ConnectionDetails {
    pub ip_address: String,
    pub router: String,
    pub signal_strength: i32,
    pub security: String,
}
