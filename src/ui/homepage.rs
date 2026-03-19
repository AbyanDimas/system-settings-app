use iced::{
    widget::{button, column, container, row, text, svg, scrollable},
    Alignment, Element, Length, Color,
};

use crate::app::{App, Message, Route};
use crate::ui::icons;
use crate::theme::card;

struct HomeListItem {
    label: &'static str,
    route: Route,
    icon: iced::widget::svg::Handle,
    color: Color,
}

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let group1 = vec![
        HomeListItem { label: "Appearance", route: Route::Appearance, icon: icons::icon_appearance(), color: Color::from_rgb8(28, 28, 30) },
        HomeListItem { label: "Displays", route: Route::Display, icon: icons::icon_display(), color: Color::from_rgb8(0, 122, 255) },
        HomeListItem { label: "Network", route: Route::Network, icon: icons::icon_network(), color: Color::from_rgb8(0, 122, 255) },
        HomeListItem { label: "Bluetooth", route: Route::Bluetooth, icon: icons::icon_bluetooth(), color: Color::from_rgb8(0, 122, 255) },
    ];

    let group2 = vec![
        HomeListItem { label: "Sound", route: Route::Audio, icon: icons::icon_audio(), color: Color::from_rgb8(255, 59, 48) },
        HomeListItem { label: "Battery & Power", route: Route::Power, icon: icons::icon_power(), color: Color::from_rgb8(52, 199, 89) },
    ];

    let group3 = vec![
        HomeListItem { label: "Startup Apps", route: Route::Startup, icon: icons::icon_rocket(), color: Color::from_rgb8(255, 149, 0) },
        HomeListItem { label: "Keybinds", route: Route::Keybinds, icon: icons::icon_keybinds(), color: Color::from_rgb8(142, 142, 147) },
        HomeListItem { label: "General", route: Route::System, icon: icons::icon_system(), color: Color::from_rgb8(142, 142, 147) },
    ];

    let content = column![
        text("System Overview").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        build_card_group(theme, group1),
        build_card_group(theme, group2),
        build_card_group(theme, group3),
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

fn build_card_group<'a>(theme: &'a crate::theme::MacOSTheme, items: Vec<HomeListItem>) -> Element<'a, Message> {
    let mut col = column![];
    let len = items.len();

    for (i, item) in items.into_iter().enumerate() {
        let icon_box = container(
            svg(item.icon)
                .width(14)
                .height(14)
                .style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) })
        )
        .width(26)
        .height(26)
        .center_x(26)
        .center_y(26)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(item.color)),
            border: iced::Border { radius: 6.0.into(), ..Default::default() },
            ..Default::default()
        });

        let content = row![
            icon_box,
            text(item.label).size(14).width(Length::Fill).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
            svg(icons::icon_chevron_right())
                .width(16)
                .height(16)
                .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
        ]
        .spacing(15)
        .align_y(Alignment::Center);

        let btn = button(content)
            .width(Length::Fill)
            .padding([12, 15])
            .on_press(Message::Navigate(item.route))
            .style(move |_, status| {
                let mut style = button::Style::default();
                style.background = match status {
                    button::Status::Hovered => Some(iced::Background::Color(
                        if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } 
                        else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                    )),
                    _ => None,
                };
                style
            });

        col = col.push(btn);

        if i < len - 1 {
            col = col.push(
                container(iced::widget::horizontal_rule(1))
                    .padding(iced::Padding { top: 0.0, right: 15.0, bottom: 0.0, left: 56.0 })
            );
        }
    }

    card(theme, col.spacing(0)).into()
}
