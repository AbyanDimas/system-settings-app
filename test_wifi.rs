use std::process::Command;
fn main() {
    let output = Command::new("iwctl").args(["device", "list"]).output().unwrap();
    let s = String::from_utf8_lossy(&output.stdout).to_string();
    println!("RAW: {:?}", s);
    for line in s.lines() {
        if line.contains("wlan0") {
            println!("Contains wlan0: {}", line);
            println!("Contains on: {}", line.contains("on"));
        }
    }
}
