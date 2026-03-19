use iced::{
    widget::{container, Container}, Border, Color, Element, Shadow, Theme,
};

#[derive(Default, Clone, Copy)]
pub enum MacOSTheme {
    #[default]
    Light,
    Dark,
}

impl MacOSTheme {
    pub fn is_dark(&self) -> bool {
        matches!(self, MacOSTheme::Dark)
    }

    pub fn bg(&self) -> Color {
        if self.is_dark() {
            color(0x1C, 0x1C, 0x1E)
        } else {
            color(0xF5, 0xF5, 0xF7)
        }
    }

    pub fn card_bg(&self) -> Color {
        if self.is_dark() {
            color(0x2C, 0x2C, 0x2E)
        } else {
            color(0xFF, 0xFF, 0xFF)
        }
    }

    pub fn sidebar_bg(&self) -> Color {
        if self.is_dark() {
            color(0x24, 0x24, 0x26)
        } else {
            color(0xEB, 0xEB, 0xEB)
        }
    }

    pub fn accent(&self) -> Color {
        color(0x00, 0x7A, 0xFF)
    }

    pub fn text_primary(&self) -> Color {
        if self.is_dark() {
            color(0xF5, 0xF5, 0xF7)
        } else {
            color(0x1D, 0x1D, 0x1F)
        }
    }

    pub fn text_secondary(&self) -> Color {
        color(0x6E, 0x6E, 0x73)
    }

    pub fn border(&self) -> Color {
        if self.is_dark() {
            color(0x3A, 0x3A, 0x3C)
        } else {
            color(0xD2, 0xD2, 0xD7)
        }
    }

    pub fn iced_theme(&self) -> Theme {
        // Fallback to basic theme
        if self.is_dark() {
            Theme::Dark
        } else {
            Theme::Light
        }
    }
}

// Helper to convert hex to color
const fn color(r: u8, g: u8, b: u8) -> Color {
    Color::from_rgb(r as f32 / 255.0, g as f32 / 255.0, b as f32 / 255.0)
}

// Reusable card container styled like macOS
pub fn card<'a, Message>(
    theme: &MacOSTheme,
    content: impl Into<Element<'a, Message>>,
) -> Container<'a, Message> {
    let bg = theme.card_bg();
    let border_color = theme.border();
    
    container(content)
        .padding(16)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(bg)),
            border: Border {
                color: border_color,
                width: 1.0,
                radius: 12.0.into(),
            },
            shadow: Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.05),
                offset: iced::Vector::new(0.0, 2.0),
                blur_radius: 8.0,
            },
            ..Default::default()
        })
}
