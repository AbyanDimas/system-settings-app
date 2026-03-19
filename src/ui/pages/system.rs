use iced::{
    widget::{button, column, container, row, text, svg, scrollable, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message, Route};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let section_1 = vec![
        ("About".to_string(), format!("{} - {}", app.sys_info.as_ref().map(|s| s.os.as_str()).unwrap_or("Omarchy"), app.sys_info.as_ref().map(|s| s.kernel.as_str()).unwrap_or("Linux")), Route::About, icons::icon_info(), Color::from_rgb8(142, 142, 147)),
        ("Software Update".to_string(), "Check for system and package updates".to_string(), Route::SoftwareUpdate, icons::icon_refresh(), Color::from_rgb8(0, 122, 255)),
        ("Storage".to_string(), format!("Used: {} / Total: {}", app.sys_info.as_ref().map(|s| s.disk_used.as_str()).unwrap_or("0"), app.sys_info.as_ref().map(|s| s.disk_total.as_str()).unwrap_or("0")), Route::Storage, icons::icon_hard_drive(), Color::from_rgb8(255, 59, 48)),
    ];

    let section_2 = vec![
        ("AirDrop & Handoff".to_string(), "Continuity and sharing across devices".to_string(), Route::Network, icons::icon_share(), Color::from_rgb8(0, 122, 255)),
        ("Accessibility".to_string(), "Vision, hearing, and motor settings".to_string(), Route::System, icons::icon_accessibility(), Color::from_rgb8(88, 86, 214)),
    ];

    let section_3 = vec![
        ("Login Items".to_string(), "Manage apps that start at login".to_string(), Route::Startup, icons::icon_rocket(), Color::from_rgb8(255, 149, 0)),
        ("Game Mode".to_string(), "Optimize system for gaming performance".to_string(), Route::GameMode, icons::icon_gamepad(), Color::from_rgb8(52, 199, 89)),
    ];

    let section_4 = vec![
        ("Language & Region".to_string(), "System language and formats".to_string(), Route::LanguageRegion, icons::icon_globe(), Color::from_rgb8(142, 142, 147)),
        ("Date & Time".to_string(), "Timezone and clock settings".to_string(), Route::DateTime, icons::icon_clock(), Color::from_rgb8(142, 142, 147)),
    ];

    let section_5 = vec![
        ("Sharing".to_string(), format!("Hostname: {}", app.sys_info.as_ref().map(|s| s.hostname.as_str()).unwrap_or("unknown")), Route::About, icons::icon_users(), Color::from_rgb8(142, 142, 147)),
        ("Time Machine".to_string(), "Backup and restore your data".to_string(), Route::Storage, icons::icon_refresh(), Color::from_rgb8(0, 122, 255)),
    ];

    let render_section = |items: Vec<(String, String, Route, iced::widget::svg::Handle, Color)>| {
        let mut list_col = column![].spacing(0);
        let len = items.len();

        for (i, (label, desc, route, icon_handle, icon_color)) in items.into_iter().enumerate() {
            let is_first = i == 0;
            let is_last = i == len - 1;
            let route_clone = route.clone();

            let icon_box = container(
                svg(icon_handle)
                    .width(16)
                    .height(16)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) })
            )
            .width(28)
            .height(28)
            .center_x(28)
            .center_y(28)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(icon_color)),
                border: iced::Border { radius: 6.0.into(), ..Default::default() },
                ..Default::default()
            });

            let btn_content = row![
                icon_box,
                column![
                    text(label).size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text(desc).size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                svg(icons::icon_chevron_right())
                    .width(14)
                    .height(14)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
            ]
            .spacing(12)
            .align_y(Alignment::Center);

            let btn = button(btn_content)
                .width(Length::Fill)
                .padding([10, 15])
                .on_press(Message::Navigate(route_clone))
                .style(move |_, status| {
                    let mut s = button::Style::default();
                    s.background = match status {
                        button::Status::Hovered => Some(iced::Background::Color(
                            if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) }
                            else { Color::from_rgba(0.0, 0.0, 0.0, 0.02) }
                        )),
                        _ => None,
                    };
                    s.border.radius = if len == 1 {
                        10.0.into()
                    } else if is_first {
                        iced::border::Radius { top_left: 10.0, top_right: 10.0, bottom_right: 0.0, bottom_left: 0.0 }
                    } else if is_last {
                        iced::border::Radius { top_left: 0.0, top_right: 0.0, bottom_right: 10.0, bottom_left: 10.0 }
                    } else {
                        0.0.into()
                    };
                    s
                });

            list_col = list_col.push(btn);
            
            if !is_last {
                list_col = list_col.push(
                    container(iced::widget::horizontal_rule(1))
                        .padding(iced::Padding { top: 0.0, right: 0.0, bottom: 0.0, left: 55.0 })
                );
            }
        }
        card(theme, list_col)
    };

    let content = column![
        text("General").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        
        render_section(section_1),
        render_section(section_2),
        render_section(section_3),
        render_section(section_4),
        render_section(section_5),
        Space::with_height(20),
    ]
    .spacing(20)
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
