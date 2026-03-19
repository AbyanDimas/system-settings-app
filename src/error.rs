use thiserror::Error;

#[derive(Error, Debug, Clone)]
pub enum AppError {
    #[error("Hyprland IPC error: {0}")]
    Hyprland(String),
    #[error("DBus error: {0}")]
    DBus(String),
    #[error("Config error: {0}")]
    Config(String),
    #[error("System error: {0}")]
    System(String),
    #[error("Unknown error: {0}")]
    Unknown(String),
}
