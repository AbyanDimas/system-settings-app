use iced::{
    widget::{column, row, text, container, toggler, text_input, pick_list, scrollable, button, svg, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;
    let is_password_valid = app.hotspot_password.len() >= 8;

    // ── Header ──────────────────────────────────────────────────────────────
    let badge: Element<'_, Message> = if app.hotspot_enabled {
        container(
            row![
                container(Space::with_width(8)).height(8)
                    .style(|_: &iced::Theme| container::Style {
                        background: Some(iced::Background::Color(Color::from_rgb8(52, 199, 89))),
                        border: iced::Border { radius: 4.0.into(), ..Default::default() },
                        ..Default::default()
                    }),
                text("Active").size(12)
                    .style(|_: &iced::Theme| text::Style { color: Some(Color::from_rgb8(52, 199, 89)), ..Default::default() }),
            ].spacing(8).align_y(Alignment::Center)
        )
        .padding([6, 12])
        .style(move |_: &iced::Theme| container::Style {
            background: Some(iced::Background::Color(Color::from_rgba(52.0/255.0, 199.0/255.0, 89.0/255.0, 0.12))),
            border: iced::Border { radius: 10.0.into(), color: Color::from_rgba(52.0/255.0, 199.0/255.0, 89.0/255.0, 0.3), width: 1.0 },
            ..Default::default()
        })
        .into()
    } else {
        Space::with_width(0).into()
    };
    let header = row![
        column![
            text("Hotspot").size(28)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            text("Share your connection via Wi-Fi").size(13)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ].spacing(3).width(Length::Fill),
        badge,
    ].align_y(Alignment::Center).spacing(12);

    // ── Main Hotspot Enable Card ─────────────────────────────────────────────
    let enable_row = row![
        container(
            svg(icons::icon_hotspot())
                .width(22).height(22)
                .style(move |_: &iced::Theme, _| iced::widget::svg::Style {
                    color: Some(if app.hotspot_enabled { Color::WHITE } else { Color::WHITE })
                })
        )
        .padding(9)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(
                if app.hotspot_enabled { Color::from_rgb8(52, 199, 89) } else { Color::from_rgb8(142, 142, 147) }
            )),
            border: iced::Border { radius: 8.0.into(), ..Default::default() },
            ..Default::default()
        }),
        column![
            text("Mobile Hotspot").size(15)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            text(if app.hotspot_enabled {
                format!("Broadcasting: {}", app.hotspot_ssid)
            } else {
                "Share this device's internet connection".to_string()
            })
            .size(12)
            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ].spacing(2).width(Length::Fill),
        toggler(app.hotspot_enabled)
            .on_toggle(move |enabled| {
                if enabled && !is_password_valid {
                    Message::Tick(())
                } else {
                    Message::ToggleHotspot(enabled)
                }
            }),
    ].spacing(14).align_y(Alignment::Center);

    // ── Password warning ────────────────────────────────────────────────────
    let pw_warning: Element<'_, Message> = if !is_password_valid && !app.hotspot_password.is_empty() {
        container(
            row![
                svg(icons::icon_alert_triangle()).width(13).height(13)
                    .style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(255, 149, 0)) }),
                text("Password must be at least 8 characters").size(12)
                    .style(|_: &iced::Theme| text::Style { color: Some(Color::from_rgb8(255, 149, 0)), ..Default::default() }),
            ].spacing(8).align_y(Alignment::Center)
        )
        .padding([4, 0])
        .into()
    } else {
        Space::with_height(0).into()
    };

    // ── Settings Section ─────────────────────────────────────────────────────
    let settings_col = column![
        // Network name
        row![
            text("Network Name").size(13).width(Length::Fixed(145.0))
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            text_input("e.g. Omarchy Hotspot", &app.hotspot_ssid)
                .on_input(Message::HotspotSsidChanged)
                .size(14)
                .width(Length::Fill),
        ].align_y(Alignment::Center).spacing(12),

        container(iced::widget::horizontal_rule(1)).padding([5, 0]),

        // Password
        row![
            text("Password").size(13).width(Length::Fixed(145.0))
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            column![
                text_input("Min. 8 characters", &app.hotspot_password)
                    .secure(true)
                    .on_input(Message::HotspotPasswordChanged)
                    .size(14)
                    .width(Length::Fill),
                pw_warning,
            ].spacing(4).width(Length::Fill),
        ].align_y(Alignment::Start).spacing(12),

        container(iced::widget::horizontal_rule(1)).padding([5, 0]),

        // Security
        row![
            text("Security").size(13).width(Length::Fixed(145.0))
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            pick_list(
                vec!["WPA2".to_string(), "WPA3".to_string(), "Open".to_string()],
                Some(app.hotspot_security.clone()),
                Message::HotspotSecurityChanged
            )
            .width(Length::Fill),
        ].align_y(Alignment::Center).spacing(12),

        container(iced::widget::horizontal_rule(1)).padding([5, 0]),

        // Save button
        row![
            Space::with_width(Length::Fill),
            button(
                row![
                    svg(icons::icon_check()).width(13).height(13)
                        .style(|_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) }),
                    text("Save Settings").size(13)
                        .style(|_: &iced::Theme| text::Style { color: Some(Color::WHITE), ..Default::default() }),
                ].spacing(8).align_y(Alignment::Center)
            )
            .padding([8, 20])
            .on_press(if is_password_valid { Message::SaveHotspotSettings } else { Message::Tick(()) })
            .style(move |_, status| {
                let mut s = button::Style::default();
                let base = if is_password_valid { theme.accent() } else { theme.text_secondary() };
                s.background = Some(iced::Background::Color(match status {
                    button::Status::Hovered if is_password_valid => Color::from_rgb8(0, 100, 220),
                    _ => base,
                }));
                s.text_color = Color::WHITE;
                s.border.radius = 8.0.into();
                s
            }),
        ],
    ].spacing(14);

    let settings_card: Element<'_, Message> = card(theme,
        column![
            enable_row,
            container(iced::widget::horizontal_rule(1)).padding([5, 0]),
            settings_col,
        ].spacing(14).padding([2, 0])
    ).into();

    // ── Active Hotspot Info Banner ────────────────────────────────────────────
    let active_info: Element<'_, Message> = if app.hotspot_enabled {
        card(theme,
            column![
                row![
                    svg(icons::icon_info()).width(15).height(15)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }),
                    text("Hotspot is active").size(14)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                ].spacing(10).align_y(Alignment::Center),
                // Connection credentials summary
                container(
                    column![
                        row![
                            text("Network").size(12).width(Length::Fixed(90.0))
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            text(&app.hotspot_ssid).size(13)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        ].spacing(8),
                        row![
                            text("Security").size(12).width(Length::Fixed(90.0))
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            text(&app.hotspot_security).size(13)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        ].spacing(8),
                        row![
                            text("Band").size(12).width(Length::Fixed(90.0))
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            text("2.4 GHz").size(13)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        ].spacing(8),
                    ].spacing(8)
                )
                .padding([12, 16])
                .style(move |_| container::Style {
                    background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.04) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.03) })),
                    border: iced::Border { radius: 8.0.into(), ..Default::default() },
                    ..Default::default()
                }),
            ].spacing(12)
        ).into()
    } else {
        Space::with_height(0).into()
    };

    // ── Connected Devices ────────────────────────────────────────────────────
    let devices_section: Element<'_, Message> = if app.hotspot_enabled {
        let device_count = app.hotspot_devices.len();

        let mut devices_col = column![
            row![
                svg(icons::icon_users()).width(14).height(14)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                text(format!("Connected Devices ({})", device_count)).size(12)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(8).align_y(Alignment::Center),
        ].spacing(10);

        let devices_card: Element<'_, Message> = if app.hotspot_devices.is_empty() {
            card(theme,
                container(
                    column![
                        svg(icons::icon_smartphone()).width(28).height(28)
                            .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                        text("No devices connected").size(14)
                            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                        text("Devices will appear here when they join your hotspot").size(11)
                            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].spacing(8).align_x(Alignment::Center)
                )
                .width(Length::Fill)
                .padding(28)
                .center_x(Length::Fill)
            ).into()
        } else {
            let mut col = column![].spacing(0);
            for (i, device) in app.hotspot_devices.iter().enumerate() {
                if i > 0 { col = col.push(container(iced::widget::horizontal_rule(1)).padding([0, 15])); }
                col = col.push(
                    row![
                        container(
                            svg(icons::icon_smartphone())
                                .width(18).height(18)
                                .style(move |_: &iced::Theme, _| iced::widget::svg::Style { color: Some(Color::WHITE) })
                        )
                        .padding(8)
                        .style(|_: &iced::Theme| container::Style {
                            background: Some(iced::Background::Color(Color::from_rgb8(0, 122, 255))),
                            border: iced::Border { radius: 8.0.into(), ..Default::default() },
                            ..Default::default()
                        }),
                        column![
                            text(&device.mac).size(14)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                            row![
                                container(Space::with_width(6)).height(6)
                                    .style(|_: &iced::Theme| container::Style {
                                        background: Some(iced::Background::Color(Color::from_rgb8(52, 199, 89))),
                                        border: iced::Border { radius: 3.0.into(), ..Default::default() },
                                        ..Default::default()
                                    }),
                                text(format!("Connected • {}", device.connected_time)).size(11)
                                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            ].spacing(6).align_y(Alignment::Center),
                        ].spacing(3),
                    ].spacing(14).padding([12, 16]).align_y(Alignment::Center)
                );
            }
            card(theme, col.padding([3, 0])).into()
        };

        devices_col = devices_col.push(devices_card);
        devices_col.into()
    } else {
        // Hotspot is off — show tips
        card(theme,
            column![
                row![
                    svg(icons::icon_info()).width(15).height(15)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                    text("How to use Hotspot").size(14)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                ].spacing(10).align_y(Alignment::Center),
                column![
                    tip_row("1", "Enter a network name and password (min. 8 chars)", theme),
                    tip_row("2", "Toggle the Hotspot switch to start broadcasting", theme),
                    tip_row("3", "Connect your other devices using the credentials above", theme),
                    tip_row("4", "Connected devices will appear in the list below", theme),
                ].spacing(10),
                container(
                    text("⚠  Turning on hotspot may disconnect your current Wi-Fi.")
                        .size(11)
                        .style(move |_: &iced::Theme| text::Style { color: Some(Color::from_rgb8(255, 149, 0)), ..Default::default() })
                ).padding([8, 0]),
            ].spacing(14)
        ).into()
    };

    // ── Compose ──────────────────────────────────────────────────────────────
    let content = column![
        header,
        settings_card,
        active_info,
        devices_section,
    ]
    .spacing(22)
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

fn tip_row<'a>(num: &'a str, tip: &'a str, theme: &'a crate::theme::MacOSTheme) -> Element<'a, Message> {
    row![
        container(
            text(num).size(11)
                .style(|_: &iced::Theme| text::Style { color: Some(Color::WHITE), ..Default::default() })
        )
        .padding([2, 7])
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(theme.accent())),
            border: iced::Border { radius: 10.0.into(), ..Default::default() },
            ..Default::default()
        }),
        text(tip).size(13)
            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
    ].spacing(12).align_y(Alignment::Center).into()
}
