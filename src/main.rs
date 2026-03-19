mod app;
mod backend;
mod error;
mod theme;
mod ui;

use iced::{application, Size};

pub fn main() -> iced::Result {
    application("System Settings", app::App::update, app::App::view)
        .window_size(Size::new(1100.0, 720.0))
        .theme(|app: &app::App| app.theme.iced_theme())
        .subscription(app::App::subscription)
        .run()
}
