use crate::error::AppError;
use std::process::Command;

pub struct DisplayService {}

impl DisplayService {
    pub fn new() -> Result<Self, AppError> {
        Ok(Self {})
    }

    pub fn get_brightness(&self) -> Result<u32, AppError> {
        let output = Command::new("brightnessctl")
            .args(["g"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let current = String::from_utf8_lossy(&output.stdout).trim().parse::<u32>()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let output = Command::new("brightnessctl")
            .args(["m"])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        let max = String::from_utf8_lossy(&output.stdout).trim().parse::<u32>()
            .map_err(|e| AppError::Unknown(e.to_string()))?;

        if max == 0 { return Ok(0); }
        Ok((current * 100) / max)
    }

    pub fn set_brightness(&self, percent: u32) -> Result<(), AppError> {
        Command::new("brightnessctl")
            .args(["s", &format!("{}%", percent)])
            .output()
            .map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }
}
