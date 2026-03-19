use crate::error::AppError;
use std::process::Command;
use std::fs;

#[derive(Debug, Clone)]
pub struct StorageDetails {
    pub disk_name: String,
    pub apps: String,
    pub documents: String,
    pub music: String,
    pub trash: String,
    pub system_data: String,
    pub other: String,
    pub downloads: String,
    pub pictures: String,
    pub videos: String,
    
    // Numeric values for the bar (in GB)
    pub apps_gb: f32,
    pub documents_gb: f32,
    pub music_gb: f32,
    pub trash_gb: f32,
    pub system_data_gb: f32,
    pub other_gb: f32,
    pub downloads_gb: f32,
    pub pictures_gb: f32,
    pub videos_gb: f32,
    pub total_gb: f32,
    pub used_gb: f32,
}

#[derive(Debug, Clone)]
pub struct SystemInfo {
    pub hostname: String,
    pub kernel: String,
    pub os: String,
    pub cpu: String,
    pub uptime: String,
    pub memory: String,
    pub disk_total: String,
    pub disk_used: String,
    pub packages: String,
    pub wm: String,
    pub storage_details: StorageDetails,
}

fn get_dir_size_gb(path: &str) -> (String, f32) {
    if !std::path::Path::new(path).exists() {
        return ("0 B".to_string(), 0.0);
    }
    
    // Use du -sb for raw bytes (more accurate)
    let raw_output = Command::new("du")
        .args(["-sb", path])
        .output();
    
    if let Ok(ro) = raw_output {
        let rs = String::from_utf8_lossy(&ro.stdout);
        let bytes = rs.split_whitespace().next().and_then(|b| b.parse::<u64>().ok()).unwrap_or(0);
        let gb = bytes as f32 / (1024.0 * 1024.0 * 1024.0);
        
        let size_str = if bytes > 1024 * 1024 * 1024 {
            format!("{:.1} GB", gb)
        } else if bytes > 1024 * 1024 {
            format!("{:.1} MB", bytes as f32 / (1024.0 * 1024.0))
        } else if bytes > 1024 {
            format!("{:.1} KB", bytes as f32 / 1024.0)
        } else {
            format!("{} B", bytes)
        };
        
        (size_str, gb)
    } else {
        ("0 B".to_string(), 0.0)
    }
}

fn parse_human_to_gb(s: &str) -> f32 {
    let s = s.trim();
    if s.is_empty() || s == "Unknown" { return 0.0; }
    let val_str: String = s.chars().take_while(|c| c.is_digit(10) || *c == '.').collect();
    let val = val_str.parse::<f32>().unwrap_or(0.0);
    if s.contains('G') || s.contains('g') { val }
    else if s.contains('M') || s.contains('m') { val / 1024.0 }
    else if s.contains('K') || s.contains('k') { val / (1024.0 * 1024.0) }
    else if s.contains('T') || s.contains('t') { val * 1024.0 }
    else { val / (1024.0 * 1024.0 * 1024.0) }
}

pub fn get_system_info() -> Result<SystemInfo, AppError> {
    let hostname = fs::read_to_string("/etc/hostname")
        .map(|s| s.trim().to_string())
        .unwrap_or_else(|_| "Unknown".into());

    let kernel = Command::new("uname")
        .arg("-r")
        .output()
        .map(|o| String::from_utf8_lossy(&o.stdout).trim().to_string())
        .unwrap_or_else(|_| "Unknown".into());

    let os = fs::read_to_string("/etc/os-release")
        .unwrap_or_default()
        .lines()
        .find(|l| l.starts_with("PRETTY_NAME="))
        .and_then(|l| l.split('=').nth(1))
        .map(|s| s.trim_matches('"').to_string())
        .unwrap_or_else(|| "Arch Linux".into());

    let cpu = fs::read_to_string("/proc/cpuinfo")
        .unwrap_or_default()
        .lines()
        .find(|l| l.starts_with("model name"))
        .and_then(|l| l.split(':').nth(1))
        .map(|s| s.trim().to_string())
        .unwrap_or_else(|| "Unknown".into());

    let uptime = Command::new("uptime")
        .arg("-p")
        .output()
        .map(|o| String::from_utf8_lossy(&o.stdout).trim().trim_start_matches("up ").to_string())
        .unwrap_or_else(|_| "Unknown".into());

    let memory = Command::new("free")
        .arg("-h")
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout);
            s.lines()
                .find(|l| l.starts_with("Mem:"))
                .and_then(|l| {
                    let parts: Vec<&str> = l.split_whitespace().collect();
                    if parts.len() >= 3 {
                        Some(format!("{} / {}", parts[2], parts[1]))
                    } else {
                        None
                    }
                })
                .unwrap_or_else(|| "Unknown".into())
        })
        .unwrap_or_else(|_| "Unknown".into());

    let (disk_total, disk_used) = Command::new("df")
        .args(["-h", "/"])
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout);
            s.lines()
                .nth(1)
                .and_then(|l| {
                    let parts: Vec<&str> = l.split_whitespace().collect();
                    if parts.len() >= 4 {
                        Some((parts[1].to_string(), parts[2].to_string()))
                    } else {
                        None
                    }
                })
                .unwrap_or_else(|| ("Unknown".into(), "Unknown".into()))
        })
        .unwrap_or_else(|_| ("Unknown".into(), "Unknown".into()));

    // Get disk name (vendor/model)
    let disk_name = Command::new("lsblk")
        .args(["-d", "-o", "MODEL"])
        .output()
        .ok()
        .and_then(|o| {
            String::from_utf8_lossy(&o.stdout)
                .lines()
                .nth(1) // Skip header
                .map(|s| s.trim().to_string())
        })
        .filter(|s| !s.is_empty())
        .unwrap_or_else(|| "Local Disk".into());

    // Detailed storage categories
    let home = std::env::var("HOME").unwrap_or_else(|_| "/root".into());
    let (apps_str, apps_gb) = get_dir_size_gb("/usr");
    let (docs_str, docs_gb) = get_dir_size_gb(&format!("{}/Documents", home));
    let (music_str, music_gb) = get_dir_size_gb(&format!("{}/Music", home));
    let (trash_str, trash_gb) = get_dir_size_gb(&format!("{}/.local/share/Trash", home));
    let (downloads_str, downloads_gb) = get_dir_size_gb(&format!("{}/Downloads", home));
    let (pics_str, pics_gb) = get_dir_size_gb(&format!("{}/Pictures", home));
    let (videos_str, videos_gb) = get_dir_size_gb(&format!("{}/Videos", home));
    
    let total_gb = parse_human_to_gb(&disk_total);
    let used_gb = parse_human_to_gb(&disk_used);
    
    // System data: kernel, initrd, boot, etc.
    let (boot_str, boot_gb) = get_dir_size_gb("/boot");
    let (etc_str, etc_gb) = get_dir_size_gb("/etc");
    let (var_str, var_gb) = get_dir_size_gb("/var");
    
    let system_data_gb = boot_gb + etc_gb + var_gb;
    let categorized_gb = apps_gb + docs_gb + music_gb + trash_gb + downloads_gb + pics_gb + videos_gb + system_data_gb;
    let other_gb = (used_gb - categorized_gb).max(0.0);

    let storage_details = StorageDetails {
        disk_name,
        apps: apps_str,
        documents: docs_str,
        music: music_str,
        trash: trash_str,
        system_data: format!("{:.1} GB", system_data_gb),
        other: format!("{:.1} GB", other_gb),
        downloads: downloads_str,
        pictures: pics_str,
        videos: videos_str,
        apps_gb,
        documents_gb: docs_gb,
        music_gb,
        trash_gb,
        system_data_gb,
        other_gb,
        downloads_gb,
        pictures_gb: pics_gb,
        videos_gb,
        total_gb,
        used_gb,
    };

    let packages = Command::new("sh")
        .args(["-c", "pacman -Qq | wc -l"])
        .output()
        .map(|o| String::from_utf8_lossy(&o.stdout).trim().to_string())
        .unwrap_or_else(|_| "Unknown".into());

    let wm = std::env::var("XDG_CURRENT_DESKTOP")
        .or_else(|_| std::env::var("WAYLAND_DISPLAY"))
        .unwrap_or_else(|_| "Hyprland".into());

    Ok(SystemInfo {
        hostname,
        kernel,
        os,
        cpu,
        uptime,
        memory,
        disk_total,
        disk_used,
        packages,
        wm,
        storage_details,
    })
}

pub fn empty_trash() -> Result<(), std::io::Error> {
    let home = std::env::var("HOME").unwrap_or_else(|_| "/root".into());
    let trash_path = format!("{}/.local/share/Trash", home);
    if std::path::Path::new(&trash_path).exists() {
        // We delete subfolders files and info
        let _ = Command::new("rm").args(["-rf", &format!("{}/files", trash_path)]).output();
        let _ = Command::new("rm").args(["-rf", &format!("{}/info", trash_path)]).output();
        let _ = fs::create_dir_all(format!("{}/files", trash_path));
        let _ = fs::create_dir_all(format!("{}/info", trash_path));
    }
    Ok(())
}

pub fn optimize_storage() -> Result<(), std::io::Error> {
    let home = std::env::var("HOME").unwrap_or_else(|_| "/root".into());
    let cache_path = format!("{}/.cache", home);
    // Be careful with cache, maybe just clear some known large ones or old ones
    // For now, let's just clear thumbnails as a safe example
    let _ = Command::new("rm").args(["-rf", &format!("{}/thumbnails", cache_path)]).output();
    // Clear journal logs older than 2 days if possible (requires sudo usually, so maybe not)
    Ok(())
}
