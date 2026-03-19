use iced::{
    widget::{column, row, text, container, toggler, scrollable, button, svg, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // ── Header ──────────────────────────────────────────────────────────────
    let header = row![
        column![
            text("Wi-Fi").size(28).style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_primary()), ..Default::default()
            }),
            text("Wireless Network").size(13).style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_secondary()), ..Default::default()
            }),
        ].spacing(3).width(Length::Fill),
        // Scan button always visible in header
        button(
            row![
                svg(icons::icon_refresh())
                    .width(14).height(14)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(if app.is_wifi_scanning { theme.accent() } else { theme.text_secondary() }) }),
                text(if app.is_wifi_scanning { "Scanning..." } else { "Scan" }).size(13)
                    .style(move |_: &iced::Theme| text::Style { color: Some(if app.is_wifi_scanning { theme.accent() } else { theme.text_primary() }), ..Default::default() }),
            ].spacing(8).align_y(Alignment::Center)
        )
        .padding([6, 14])
        .on_press(Message::ScanWifi)
        .style(move |_, status| {
            let mut s = button::Style::default();
            s.background = Some(iced::Background::Color(match status {
                button::Status::Hovered => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.12) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.08) },
                _ => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.07) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) },
            }));
            s.border.radius = 8.0.into();
            s.text_color = theme.text_primary();
            s
        }),
    ].align_y(Alignment::Center).spacing(15);

    // ── Wi-Fi Main Toggle Card ───────────────────────────────────────────────
    let main_row = row![
        container(
            svg(icons::icon_wifi())
                .width(20).height(20)
                .style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) })
        )
        .padding(9)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(Color::from_rgb8(0, 122, 255))),
            border: iced::Border { radius: 8.0.into(), ..Default::default() },
            ..Default::default()
        }),
        column![
            text("Wi-Fi").size(15)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            text(if app.wifi_on { if app.current_ssid == "Not Connected" { "On — Not connected" } else { "On" } } else { "Off" })
                .size(12)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ].spacing(2).width(Length::Fill),
        toggler(app.wifi_on).on_toggle(Message::SetWifi),
    ].spacing(14).align_y(Alignment::Center).padding([14, 16]);

    // ── Connected Network Banner (if connected) ──────────────────────────────
    let connected_banner: Element<'_, Message> = if app.wifi_on && app.current_ssid != "Not Connected" {
        column![
            container(iced::widget::horizontal_rule(1)).padding([0, 0]),
            container(
                row![
                    // Signal strength visualization
                    signal_bars_widget(theme, -55), // approximate
                    column![
                        row![
                            text(&app.current_ssid).size(15)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                            Space::with_width(8),
                            container(
                                text("Connected").size(10)
                                    .style(|_| text::Style { color: Some(Color::WHITE), ..Default::default() })
                            )
                            .padding([2, 8])
                            .style(|_| container::Style {
                                background: Some(iced::Background::Color(Color::from_rgb8(52, 199, 89))),
                                border: iced::Border { radius: 10.0.into(), ..Default::default() },
                                ..Default::default()
                            }),
                        ].align_y(Alignment::Center).spacing(6),
                        row![
                            if app.connection_details.ip_address.is_empty() {
                                text("Tap 'Details' for connection info").size(11)
                            } else {
                                text(format!("IP: {}", app.connection_details.ip_address)).size(11)
                            }
                            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                        ],
                    ].spacing(4).width(Length::Fill),
                    button(text("Details").size(12))
                        .padding([5, 13])
                        .on_press(Message::OpenNetworkAdvancedModal)
                        .style(move |_, status| {
                            let mut s = button::Style::default();
                            s.background = Some(iced::Background::Color(match status {
                                button::Status::Hovered => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.15) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) },
                                _ => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.08) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) },
                            }));
                            s.text_color = theme.text_primary();
                            s.border.radius = 8.0.into();
                            s
                        }),
                ].spacing(14).align_y(Alignment::Center).padding([14, 16])
            ),
        ].into()
    } else {
        Space::with_height(0).into()
    };

    let top_card: Element<'_, Message> = card(theme,
        column![main_row, connected_banner].spacing(0)
    ).into();

    // ── VPN & Network Security (upfront) ────────────────────────────────────
    let vpn_section: Element<'_, Message> = if !app.vpns.is_empty() {
        let mut vpn_col = column![
            row![
                svg(icons::icon_shield())
                    .width(15).height(15)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                text("VPN & Network Security").size(12)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(8).padding(iced::Padding { top: 0.0, right: 0.0, bottom: 8.0, left: 0.0 }).align_y(Alignment::Center),
        ].spacing(0);

        let mut vpn_items = column![].spacing(0);
        for (i, vpn) in app.vpns.iter().enumerate() {
            if i > 0 { vpn_items = vpn_items.push(container(iced::widget::horizontal_rule(1)).padding([0, 10])); }
            let is_ts = vpn.name.contains("Tailscale");
            let status_color = if vpn.is_running { Color::from_rgb8(52, 199, 89) } else { theme.text_secondary() };
            let vpn_ctrl: Element<'_, Message> = if is_ts {
                toggler(vpn.is_running)
                    .on_toggle(Message::ToggleTailscale)
                    .into()
            } else {
                container(
                    text(if vpn.is_running { "Active" } else { "Inactive" }).size(12)
                        .style(move |_: &iced::Theme| text::Style { color: Some(status_color), ..Default::default() })
                ).into()
            };
            let item = row![
                // Status dot
                container(Space::with_width(8)).height(8)
                    .style(move |_: &iced::Theme| container::Style {
                        background: Some(iced::Background::Color(status_color)),
                        border: iced::Border { radius: 4.0.into(), ..Default::default() },
                        ..Default::default()
                    }),
                column![
                    text(&vpn.name).size(14)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text(&vpn.status).size(11)
                        .style(move |_: &iced::Theme| text::Style { color: Some(status_color), ..Default::default() }),
                ].width(Length::Fill).spacing(2),
                vpn_ctrl,
            ].spacing(14).padding([12, 16]).align_y(Alignment::Center);
            vpn_items = vpn_items.push(item);
        }
        vpn_col = vpn_col.push(card(theme, vpn_items));
        vpn_col.into()
    } else {
        Space::with_height(0).into()
    };

    // ── Available Networks ───────────────────────────────────────────────────
    let networks_section: Element<'_, Message> = if app.wifi_on {
        let mut section = column![
            text("Available Networks").size(12)
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ].spacing(10);

        let mut hotspots: Vec<_> = Vec::new();
        let mut known: Vec<_> = Vec::new();
        let mut others: Vec<_> = Vec::new();

        for net in &app.available_networks {
            if net.ssid.to_lowercase().contains("iphone") || net.ssid.to_lowercase().contains("hotspot") || net.ssid.to_lowercase().contains("mobile") {
                hotspots.push(net);
            } else if net.is_connected || net.is_known {
                known.push(net);
            } else {
                others.push(net);
            }
        }

        if app.available_networks.is_empty() && !app.is_wifi_scanning {
            section = section.push(
                card(theme,
                    container(
                        column![
                            svg(icons::icon_wifi()).width(32).height(32)
                                .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                            text("No networks found").size(14)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            text("Tap Scan to search for networks").size(11)
                                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                        ].spacing(8).align_x(Alignment::Center)
                    )
                    .width(Length::Fill)
                    .padding(30)
                    .center_x(Length::Fill)
                )
            );
        }

        if !hotspots.is_empty() {
            section = section.push(
                text("Personal Hotspot").size(11)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            );
            let mut col = column![].spacing(0);
            for (i, net) in hotspots.iter().enumerate() {
                if i > 0 { col = col.push(container(iced::widget::horizontal_rule(1)).padding([0, 15])); }
                col = col.push(build_network_row(app, net, icons::icon_smartphone(), true));
            }
            section = section.push(card(theme, col.padding([3, 0])));
        }

        if !known.is_empty() {
            section = section.push(
                text("Saved Networks").size(11)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            );
            let mut col = column![].spacing(0);
            for (i, net) in known.iter().enumerate() {
                if i > 0 { col = col.push(container(iced::widget::horizontal_rule(1)).padding([0, 15])); }
                col = col.push(build_network_row(app, net, icons::icon_wifi(), false));
            }
            section = section.push(card(theme, col.padding([3, 0])));
        }

        if !others.is_empty() {
            section = section.push(
                text("Other Networks").size(11)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            );
            let mut col = column![].spacing(0);
            for (i, net) in others.iter().take(15).enumerate() {
                if i > 0 { col = col.push(container(iced::widget::horizontal_rule(1)).padding([0, 15])); }
                col = col.push(build_network_row(app, net, icons::icon_wifi(), false));
            }
            section = section.push(card(theme, col.padding([3, 0])));
        }

        section.into()
    } else {
        // Wi-Fi is off — show a friendly message
        card(theme,
            container(
                column![
                    svg(icons::icon_wifi_off()).width(36).height(36)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                    text("Wi-Fi is turned off").size(15)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Turn on Wi-Fi to see available networks").size(12)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(10).align_x(Alignment::Center)
            )
            .width(Length::Fill)
            .padding(35)
            .center_x(Length::Fill)
        ).into()
    };

    // ── Compose content ─────────────────────────────────────────────────────
    let content = column![
        header,
        top_card,
        vpn_section,
        networks_section,
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

// ── Signal strength bars widget (4 bars like Windows/macOS) ─────────────────
fn signal_bars_widget<'a>(theme: &'a crate::theme::MacOSTheme, _signal_dbm: i32) -> Element<'a, Message> {
    // We use signal_strength icon for simplicity since we don't have exact dBm
    container(
        svg(icons::icon_signal())
            .width(18).height(18)
            .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) })
    ).into()
}

// ── Network row ─────────────────────────────────────────────────────────────
fn build_network_row<'a>(
    app: &'a App,
    net: &'a crate::backend::network::WifiNetwork,
    icon: iced::widget::svg::Handle,
    is_hotspot: bool,
) -> Element<'a, Message> {
    let theme = &app.theme;
    let is_secured = net.security != "open" && !net.security.is_empty();

    let msg = if net.is_connected {
        // Already connected — open details
        Message::OpenNetworkAdvancedModal
    } else if net.is_known {
        Message::ConnectToKnown(net.ssid.clone())
    } else {
        Message::OpenNetworkModal(net.ssid.clone())
    };

    button(
        row![
            // Left: icon (with hotspot tint)
            container(
                svg(if is_hotspot { icons::icon_smartphone() } else { icon })
                    .width(16).height(16)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(
                        if net.is_connected { theme.accent() } else { theme.text_primary() }
                    ) })
            ).padding([0, 2]),
            // Middle: SSID + badges
            column![
                text(&net.ssid).size(14)
                    .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                if net.is_connected {
                    Element::from(text("Connected").size(11)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.accent()), ..Default::default() }))
                } else if net.is_known {
                    Element::from(text("Saved").size(11)
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }))
                } else {
                    Element::from(Space::with_height(0))
                }
            ].spacing(2).width(Length::Fill),
            // Right: lock icon + security badge
            {
                let lock_icon: Element<'_, Message> = if is_secured {
                    svg(icons::icon_lock()).width(12).height(12)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
                        .into()
                } else {
                    Space::with_width(0).into()
                };
                let security_badge: Element<'_, Message> = if !net.security.is_empty() && net.security != "open" {
                    container(
                        text(&net.security).size(10)
                            .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                    ).padding([2, 6])
                    .style(move |_: &iced::Theme| container::Style {
                        background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.07) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.06) })),
                        border: iced::Border { radius: 5.0.into(), ..Default::default() },
                        ..Default::default()
                    }).into()
                } else {
                    Space::with_width(0).into()
                };
                row![
                    lock_icon,
                    security_badge,
                    // Chevron
                    svg(icons::icon_chevron_right()).width(14).height(14)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                ].spacing(8).align_y(Alignment::Center)
            },
        ].spacing(12).align_y(Alignment::Center)
    )
    .width(Length::Fill)
    .padding([12, 16])
    .on_press(msg)
    .style(move |_, status| {
        let mut s = button::Style::default();
        s.background = match status {
            button::Status::Hovered => Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.04) })),
            _ => None,
        };
        s.text_color = theme.text_primary();
        s
    })
    .into()
}
