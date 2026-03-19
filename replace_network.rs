use std::fs;

fn main() {
    let mut content = fs::read_to_string("src/backend/network.rs").unwrap();
    // Insert get_wifi_device method
    let method = r#"
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
"#;
    content = content.replace("    pub fn is_wifi_on(&self) -> Result<bool, AppError> {", method);

    // Replace wlan0 with self.get_wifi_device()
    content = content.replace("wlan0", &format!("{{}}")); // Temporary placeholder
    
    // Manual replacements:
    // in is_wifi_on:
    let old_is_wifi_on = r#"
        // Check if wlan0 has Powered status 'on'
        for line in s.lines() {
            if line.contains("wlan0") {
                return Ok(line.contains("on") || line.contains("yes") || line.contains("powered"));
            }
        }
"#;
    let new_is_wifi_on = r#"
        let dev = self.get_wifi_device();
        for line in s.lines() {
            if line.contains(&dev) {
                return Ok(line.contains(" on ") || line.contains(" yes "));
            }
        }
"#;
    content = content.replace(old_is_wifi_on, new_is_wifi_on);
    fs::write("src/backend/network.rs", content).unwrap();
}
