use std::process::Command;
use std::fs;

#[derive(Debug, Clone)]
pub struct SystemUser {
    pub username: String,
    pub uid: u32,
    pub gid: u32,
    pub home: String,
    pub shell: String,
    pub is_sudoer: bool,
    pub last_login: String,
    pub groups: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct LoginEntry {
    pub username: String,
    pub terminal: String,
    pub from: String,
    pub date: String,
    pub status: LoginStatus,
}

#[derive(Debug, Clone, PartialEq)]
pub enum LoginStatus {
    Success,
    Failed,
}

#[derive(Debug, Clone)]
pub struct OpenPort {
    pub port: u16,
    pub protocol: String,
    pub state: String,
    pub service: String,
}

#[derive(Debug, Clone)]
pub struct SecurityRecommendation {
    pub title: String,
    pub description: String,
    pub severity: RecommendationSeverity,
}

#[derive(Debug, Clone, PartialEq)]
pub enum RecommendationSeverity {
    Low,
    Medium,
    High,
}

#[derive(Debug, Clone)]
pub struct SecurityInfo {
    pub users: Vec<SystemUser>,
    pub login_history: Vec<LoginEntry>,
    pub open_ports: Vec<OpenPort>,
    pub ssh_enabled: bool,
    pub firewall_active: bool,
    pub audit_logging_enabled: bool,
    pub failed_logins_count: u32,
    pub sudo_log_path: String,
    pub last_system_update: String,
    pub selinux_status: String,
    pub recommendations: Vec<SecurityRecommendation>,
}

/// Verify user password using sudo -v
pub fn verify_password(password: &str) -> bool {
    let child = Command::new("sudo")
        .args(["-S", "-v"])
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .spawn();

    if let Ok(mut child) = child {
        if let Some(mut stdin) = child.stdin.take() {
            use std::io::Write;
            writeln!(stdin, "{}", password).ok();
        }
        child.wait().map(|s| s.success()).unwrap_or(false)
    } else {
        false
    }
}

/// Toggle SSH service
pub fn set_ssh_enabled(password: &str, enable: bool) -> bool {
    let action = if enable { "start" } else { "stop" };
    let child = Command::new("sudo")
        .args(["-S", "systemctl", action, "sshd"])
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .spawn();

    if let Ok(mut child) = child {
        if let Some(mut stdin) = child.stdin.take() {
            use std::io::Write;
            writeln!(stdin, "{}", password).ok();
        }
        child.wait().map(|s| s.success()).unwrap_or(false)
    } else {
        false
    }
}

/// Toggle UFW firewall
pub fn set_firewall_active(password: &str, enable: bool) -> bool {
    let action = if enable { "enable" } else { "disable" };
    let child = Command::new("sudo")
        .args(["-S", "ufw", action])
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .spawn();

    if let Ok(mut child) = child {
        if let Some(mut stdin) = child.stdin.take() {
            use std::io::Write;
            writeln!(stdin, "{}", password).ok();
        }
        child.wait().map(|s| s.success()).unwrap_or(false)
    } else {
        false
    }
}

/// Toggle Audit Logging (auditd)
pub fn set_audit_logging_enabled(password: &str, enable: bool) -> bool {
    let action = if enable { "start" } else { "stop" };
    let child = Command::new("sudo")
        .args(["-S", "systemctl", action, "auditd"])
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .spawn();

    if let Ok(mut child) = child {
        if let Some(mut stdin) = child.stdin.take() {
            use std::io::Write;
            writeln!(stdin, "{}", password).ok();
        }
        child.wait().map(|s| s.success()).unwrap_or(false)
    } else {
        false
    }
}

/// Check if auditd is active
pub fn is_audit_logging_enabled() -> bool {
    Command::new("systemctl")
        .args(["is-active", "--quiet", "auditd"])
        .status()
        .map(|s| s.success())
        .unwrap_or(false)
}

/// Get all real system users (uid >= 1000, excluding nobody)
pub fn get_system_users() -> Vec<SystemUser> {
    let sudoers = get_sudoers();
    
    let passwd = fs::read_to_string("/etc/passwd").unwrap_or_default();
    let mut users = Vec::new();

    for line in passwd.lines() {
        let parts: Vec<&str> = line.split(':').collect();
        if parts.len() < 7 {
            continue;
        }
        let uid: u32 = parts[2].parse().unwrap_or(0);
        // Only real user accounts (uid 1000+) and root (uid 0)
        if uid != 0 && uid < 1000 {
            continue;
        }
        // Exclude nobody
        if parts[0] == "nobody" {
            continue;
        }

        let username = parts[0].to_string();
        let home = parts[5].to_string();
        let shell = parts[6].to_string();

        // Skip nologin/false shells (system accounts)
        if shell.contains("nologin") || shell.contains("false") {
            continue;
        }

        let is_sudoer = sudoers.contains(&username);
        let last_login = get_last_login(&username);
        let groups = get_user_groups(&username);

        users.push(SystemUser {
            username,
            uid,
            gid: parts[3].parse().unwrap_or(0),
            home,
            shell,
            is_sudoer,
            last_login,
            groups,
        });
    }

    users
}

fn get_sudoers() -> Vec<String> {
    // Check /etc/sudoers and wheel group members
    let mut sudoers = Vec::new();

    // Check wheel group
    let group = fs::read_to_string("/etc/group").unwrap_or_default();
    for line in group.lines() {
        let parts: Vec<&str> = line.split(':').collect();
        if parts.len() >= 4 && (parts[0] == "wheel" || parts[0] == "sudo") {
            let members: Vec<String> = parts[3].split(',').map(|s| s.trim().to_string()).filter(|s| !s.is_empty()).collect();
            sudoers.extend(members);
        }
    }

    sudoers
}

fn get_user_groups(username: &str) -> Vec<String> {
    Command::new("groups")
        .arg(username)
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout);
            // Output: "username : group1 group2 group3"
            s.split(':').nth(1)
                .unwrap_or("")
                .split_whitespace()
                .map(|g| g.to_string())
                .collect()
        })
        .unwrap_or_default()
}

fn get_last_login(username: &str) -> String {
    Command::new("lastlog")
        .args(["-u", username])
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout);
            // Parse last line only
            s.lines()
                .last()
                .unwrap_or("")
                .split_whitespace()
                .skip(1) // skip username
                .collect::<Vec<_>>()
                .join(" ")
                .trim()
                .to_string()
        })
        .unwrap_or_else(|_| "Never".into())
        .replace("**Never logged in**", "Never")
}

/// Get recent login history (success + failed)
pub fn get_login_history(limit: usize) -> Vec<LoginEntry> {
    let mut entries = Vec::new();

    // `last` for successful logins
    let output = Command::new("last")
        .args(["-n", &limit.to_string(), "-F", "--time-format=iso"])
        .output();

    if let Ok(o) = output {
        let s = String::from_utf8_lossy(&o.stdout);
        for line in s.lines().take(limit) {
            if line.starts_with("wtmp") || line.is_empty() {
                continue;
            }
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() < 3 {
                continue;
            }
            let username = parts[0].to_string();
            let terminal = parts[1].to_string();
            let from = if parts.len() > 2 && !parts[2].starts_with("Mon")
                && !parts[2].starts_with("Tue") && !parts[2].starts_with("Wed")
                && !parts[2].starts_with("Thu") && !parts[2].starts_with("Fri")
                && !parts[2].starts_with("Sat") && !parts[2].starts_with("Sun") {
                parts[2].to_string()
            } else {
                "local".to_string()
            };
            // Take date from remaining parts
            let date = parts.iter().skip(3).take(4).cloned().collect::<Vec<_>>().join(" ");
            
            entries.push(LoginEntry {
                username,
                terminal,
                from,
                date,
                status: LoginStatus::Success,
            });
        }
    }

    // `lastb` for failed logins (may require root, try anyway)
    let failed_output = Command::new("lastb")
        .args(["-n", "20"])
        .output();

    if let Ok(o) = failed_output {
        let s = String::from_utf8_lossy(&o.stdout);
        for line in s.lines().take(20) {
            if line.starts_with("btmp") || line.is_empty() {
                continue;
            }
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() < 2 {
                continue;
            }
            let username = parts[0].to_string();
            let terminal = parts.get(1).unwrap_or(&"?").to_string();
            let from = parts.get(2).map(|s| s.to_string()).unwrap_or_else(|| "local".into());
            let date = parts.iter().skip(3).take(4).cloned().collect::<Vec<_>>().join(" ");
            
            entries.push(LoginEntry {
                username,
                terminal,
                from,
                date,
                status: LoginStatus::Failed,
            });
        }
    }

    entries
}

/// Count failed login attempts
pub fn get_failed_login_count() -> u32 {
    Command::new("sh")
        .args(["-c", "lastb -n 100 2>/dev/null | wc -l"])
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout);
            s.trim().parse::<u32>().unwrap_or(0).saturating_sub(2) // subtract header lines
        })
        .unwrap_or(0)
}

/// Get listening ports using ss
pub fn get_open_ports() -> Vec<OpenPort> {
    let output = Command::new("ss")
        .args(["-tlunp"])
        .output();

    let mut ports = Vec::new();
    if let Ok(o) = output {
        let s = String::from_utf8_lossy(&o.stdout);
        for line in s.lines().skip(1) {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() < 5 {
                continue;
            }
            let proto = parts[0].to_string();
            let local_addr = parts[4];
            if let Some(port_str) = local_addr.rsplit(':').next() {
                if let Ok(port) = port_str.parse::<u16>() {
                    let service = get_service_name(port);
                    ports.push(OpenPort {
                        port,
                        protocol: proto,
                        state: "LISTEN".into(),
                        service,
                    });
                }
            }
        }
    }

    ports.sort_by_key(|p| p.port);
    ports.dedup_by_key(|p| p.port);
    ports
}

fn get_service_name(port: u16) -> String {
    match port {
        22 => "SSH".into(),
        80 => "HTTP".into(),
        443 => "HTTPS".into(),
        25 => "SMTP".into(),
        53 => "DNS".into(),
        3306 => "MySQL".into(),
        5432 => "PostgreSQL".into(),
        6379 => "Redis".into(),
        8080 => "HTTP-Alt".into(),
        3000 => "Dev Server".into(),
        5900..=5999 => "VNC".into(),
        _ => "Unknown".into(),
    }
}

/// Check if SSH daemon is running
pub fn is_ssh_enabled() -> bool {
    Command::new("systemctl")
        .args(["is-active", "--quiet", "sshd"])
        .status()
        .map(|s| s.success())
        .unwrap_or(false)
}

/// Check if firewall (ufw or nftables) is active (non-root safe)
pub fn is_firewall_active() -> bool {
    // Check ufw via systemctl (doesn't require root)
    let ufw_active = Command::new("systemctl")
        .args(["is-active", "--quiet", "ufw"])
        .status()
        .map(|s| s.success())
        .unwrap_or(false);

    if ufw_active {
        return true;
    }

    // Try reading ufw.conf (standard on many distros)
    if let Ok(content) = fs::read_to_string("/etc/ufw/ufw.conf") {
        if content.contains("ENABLED=yes") {
            return true;
        }
    }

    // Check nftables
    Command::new("systemctl")
        .args(["is-active", "--quiet", "nftables"])
        .status()
        .map(|s| s.success())
        .unwrap_or(false)
}

/// Get security recommendations based on current info
pub fn get_security_recommendations(info: &SecurityInfo) -> Vec<SecurityRecommendation> {
    let mut recs = Vec::new();

    if !info.firewall_active {
        recs.push(SecurityRecommendation {
            title: "Firewall is disabled".into(),
            description: "Enable the firewall to protect your system from unauthorized network access.".into(),
            severity: RecommendationSeverity::High,
        });
    }

    if info.ssh_enabled {
        recs.push(SecurityRecommendation {
            title: "SSH Server is active".into(),
            description: "Unless you need remote access, consider disabling SSH to reduce attack surface.".into(),
            severity: RecommendationSeverity::Medium,
        });
    }

    if info.failed_logins_count > 10 {
        recs.push(SecurityRecommendation {
            title: "Many failed login attempts".into(),
            description: format!("Detected {} failed logins. Consider changing your password.", info.failed_logins_count),
            severity: RecommendationSeverity::High,
        });
    }

    if info.open_ports.iter().any(|p| p.port == 21 || p.port == 23) {
        recs.push(SecurityRecommendation {
            title: "Insecure ports open".into(),
            description: "Telnet (23) or FTP (21) detected. Use SSH (22) or SFTP instead.".into(),
            severity: RecommendationSeverity::High,
        });
    }

    if !info.audit_logging_enabled {
        recs.push(SecurityRecommendation {
            title: "Audit logging disabled".into(),
            description: "Enable audit logging to keep track of security-related events.".into(),
            severity: RecommendationSeverity::Low,
        });
    }

    recs
}

/// Get last system update date
pub fn get_last_update() -> String {
    // Arch Linux logs updates in /var/log/pacman.log
    Command::new("sh")
        .args(["-c", "grep 'upgraded\\|installed' /var/log/pacman.log 2>/dev/null | tail -1 | awk '{print $1, $2}' | tr -d '[]'"])
        .output()
        .map(|o| {
            let s = String::from_utf8_lossy(&o.stdout).trim().to_string();
            if s.is_empty() { "Unknown".into() } else { s }
        })
        .unwrap_or_else(|_| "Unknown".into())
}

/// Collect all security info
pub fn get_security_info() -> SecurityInfo {
    let mut info = SecurityInfo {
        users: get_system_users(),
        login_history: get_login_history(30),
        open_ports: get_open_ports(),
        ssh_enabled: is_ssh_enabled(),
        firewall_active: is_firewall_active(),
        audit_logging_enabled: is_audit_logging_enabled(),
        failed_logins_count: get_failed_login_count(),
        sudo_log_path: "/var/log/auth.log".into(),
        last_system_update: get_last_update(),
        selinux_status: "Not applicable (Arch Linux)".into(),
        recommendations: Vec::new(),
    };
    
    info.recommendations = get_security_recommendations(&info);
    info
}
