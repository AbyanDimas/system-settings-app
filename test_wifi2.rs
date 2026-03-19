use std::process::Command;

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

fn is_wifi_on() -> bool {
    let output = Command::new("iwctl")
        .args(["device", "list"])
        .output()
        .unwrap();

    let s = strip_ansi(&String::from_utf8_lossy(&output.stdout));
    for line in s.lines() {
        if line.contains("wlan0") {
            return line.contains("on") || line.contains("yes") || line.contains("powered");
        }
    }
    false
}

fn main() {
    println!("Is wifi on? {}", is_wifi_on());
}
