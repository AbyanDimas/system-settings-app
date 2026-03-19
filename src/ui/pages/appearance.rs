use iced::{widget::text, Element};
use crate::app::{App, Message};

pub fn view(_app: &App) -> Element<'_, Message> {
    text("Appearance Page").into()
}
