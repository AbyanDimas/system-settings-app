use iced::{
    widget::{button, column, container, row, text, svg, scrollable, Space, pick_list, toggler, slider},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;


#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WaybarPosition {
    Top,
    Bottom,
    Left,
    Right,
}

impl std::fmt::Display for WaybarPosition {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}",
            match self {
                WaybarPosition::Top => "top",
                WaybarPosition::Bottom => "bottom",
                WaybarPosition::Left => "left",
                WaybarPosition::Right => "right",
            }
        )
    }
}

impl From<String> for WaybarPosition {
    fn from(s: String) -> Self {
        match s.to_lowercase().as_str() {
            "bottom" => WaybarPosition::Bottom,
            "left" => WaybarPosition::Left,
            "right" => WaybarPosition::Right,
            _ => WaybarPosition::Top,
        }
    }
}

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let Some(config) = &app.waybar_config else {
        return container(text("Loading Waybar configuration...").size(20))
            .width(Length::Fill)
            .height(Length::Fill)
            .center_x(Length::Fill)
            .center_y(Length::Fill)
            .into();
    };

    // Header with Save Button
    let header = row![
        column![
            text("Waybar").size(28).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
            text("Customize your status bar and system integration.")
                .size(14)
                .style(move |_| text::Style {
                    color: Some(theme.text_secondary()),
                    ..Default::default()
                }),
        ].spacing(8).width(Length::Fill),
        button(
            text("Save & Apply").size(13)
        )
        .padding([8, 16])
        .on_press(Message::SaveWaybarConfig)
        .style(move |_, status| {
            let mut s = button::Style::default();
            s.background = Some(iced::Background::Color(theme.accent()));
            s.text_color = Color::WHITE;
            s.border.radius = 8.0.into();
            if status == button::Status::Hovered {
                s.background = Some(iced::Background::Color(Color { a: 0.8, ..theme.accent() }));
            }
            s
        })
    ].align_y(Alignment::Center);

    // Section 1: General Settings
    let current_pos = WaybarPosition::from(config.position.clone());
    let general_settings = column![
        text("Layout").size(16).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        card(theme, column![
            row![
                text("Position").width(Length::Fill),
                pick_list(
                    &[WaybarPosition::Top, WaybarPosition::Bottom, WaybarPosition::Left, WaybarPosition::Right][..],
                    Some(current_pos),
                    |pos| {
                        let mut new_config = config.clone();
                        new_config.position = pos.to_string();
                        Message::UpdateWaybarConfig(new_config)
                    }
                ).width(Length::Fixed(120.0))
            ].align_y(Alignment::Center).spacing(20),
            
            container(iced::widget::horizontal_rule(1))
                .padding(iced::Padding { top: 10.0, right: 0.0, bottom: 10.0, left: 0.0 }),

            row![
                text("Height").width(Length::Fill),
                slider(20..=60, config.height.unwrap_or(26), |h| {
                    let mut new_config = config.clone();
                    new_config.height = Some(h);
                    Message::UpdateWaybarConfig(new_config)
                }).width(Length::Fixed(150.0)),
                text(format!("{}px", config.height.unwrap_or(26))).size(12).width(Length::Fixed(40.0))
            ].align_y(Alignment::Center).spacing(20),

            container(iced::widget::horizontal_rule(1))
                .padding(iced::Padding { top: 10.0, right: 0.0, bottom: 10.0, left: 0.0 }),

            row![
                text("Spacing").width(Length::Fill),
                slider(0..=20, config.spacing.unwrap_or(0), |s| {
                    let mut new_config = config.clone();
                    new_config.spacing = Some(s);
                    Message::UpdateWaybarConfig(new_config)
                }).width(Length::Fixed(150.0)),
                text(format!("{}px", config.spacing.unwrap_or(0))).size(12).width(Length::Fixed(40.0))
            ].align_y(Alignment::Center).spacing(20),
        ].spacing(5))
    ].spacing(12);

    // Section 2: Omarchy Integration
    let has_omarchy_menu = config.modules_left.contains(&"custom/omarchy".to_string());
    let has_tray = config.modules_right.contains(&"tray".to_string());
    let has_control_center = config.modules_right.contains(&"custom/control-center".to_string());

    let omarchy_integration = column![
        text("Omarchy Integration").size(16).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        card(theme, column![
            row![
                column![
                    text("Omarchy Menu").size(14),
                    text("Show Abyan Dimas / Omarchy logo on the left").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(has_omarchy_menu).on_toggle(|active| {
                    let mut new_config = config.clone();
                    if active {
                        if !new_config.modules_left.contains(&"custom/omarchy".to_string()) {
                            new_config.modules_left.insert(0, "custom/omarchy".to_string());
                        }
                    } else {
                        new_config.modules_left.retain(|m| m != "custom/omarchy");
                    }
                    Message::UpdateWaybarConfig(new_config)
                })
            ].align_y(Alignment::Center),

            container(iced::widget::horizontal_rule(1))
                .padding(iced::Padding { top: 10.0, right: 0.0, bottom: 10.0, left: 0.0 }),

            row![
                column![
                    text("System Tray").size(14),
                    text("Show background application icons").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(has_tray).on_toggle(|active| {
                    let mut new_config = config.clone();
                    if active {
                        if !new_config.modules_right.contains(&"tray".to_string()) {
                            new_config.modules_right.insert(0, "tray".to_string());
                        }
                    } else {
                        new_config.modules_right.retain(|m| m != "tray");
                    }
                    Message::UpdateWaybarConfig(new_config)
                })
            ].align_y(Alignment::Center),

            container(iced::widget::horizontal_rule(1))
                .padding(iced::Padding { top: 10.0, right: 0.0, bottom: 10.0, left: 0.0 }),

            row![
                column![
                    text("Control Center Toggle").size(14),
                    text("Add a button to quickly open settings").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(has_control_center).on_toggle(|active| {
                    let mut new_config = config.clone();
                    if active {
                        if !new_config.modules_right.contains(&"custom/control-center".to_string()) {
                            new_config.modules_right.push("custom/control-center".to_string());
                        }
                    } else {
                        new_config.modules_right.retain(|m| m != "custom/control-center");
                    }
                    Message::UpdateWaybarConfig(new_config)
                })
            ].align_y(Alignment::Center),
        ].spacing(5))
    ].spacing(12);

    // Section 3: Service Control
    let service_control = column![
        text("System Service").size(16).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        card(theme, row![
            column![
                text("Restart Waybar").size(14),
                text("Force Waybar to restart and reload all styles").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].width(Length::Fill),
            button(
                row![
                    svg(icons::icon_refresh()).width(14).height(14).style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) }),
                    text("Restart").size(12),
                ].spacing(8).align_y(Alignment::Center)
            )
            .padding([6, 12])
            .on_press(Message::RestartWaybar)
            .style(move |_, status| {
                let mut s = button::Style::default();
                s.background = Some(iced::Background::Color(theme.accent()));
                s.text_color = Color::WHITE;
                s.border.radius = 6.0.into();
                if status == button::Status::Hovered {
                    s.background = Some(iced::Background::Color(Color { a: 0.8, ..theme.accent() }));
                }
                s
            })
        ].align_y(Alignment::Center))
    ].spacing(12);

    let content = column![
        header,
        general_settings,
        omarchy_integration,
        service_control,
        Space::with_height(20),
    ]
    .spacing(30)
    .padding([40, 60])
    .width(Length::Fill);

    container(
        scrollable(content).direction(
            iced::widget::scrollable::Direction::Vertical(
                    iced::widget::scrollable::Scrollbar::new().width(0).scroller_width(0).margin(0)
            )
        )
    )
    .width(Length::Fill)
    .height(Length::Fill)
    .into()
}
