use iced::{
    widget::{column, row, text, container, svg, scrollable, toggler, pick_list},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::{card, MacOSTheme};
use crate::ui::icons;
use crate::backend::security::{LoginStatus, SecurityInfo, RecommendationSeverity};

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let security = &app.security_info;

    // ─── Header ───────────────────────────────────────────────────────────
    let score = security_score(security);
    let (score_color, score_label) = if score >= 80 {
        (Color::from_rgb8(52, 199, 89), "Good")
    } else if score >= 50 {
        (Color::from_rgb8(255, 214, 10), "Fair")
    } else {
        (Color::from_rgb8(255, 59, 48), "At Risk")
    };

    let header = container(
        row![
            container(
                svg(icons::icon_shield_check())
                    .width(48)
                    .height(48)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(score_color) })
            )
            .width(80)
            .height(80)
            .center_x(80)
            .center_y(80)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(Color::from_rgba(
                    score_color.r, score_color.g, score_color.b, 0.15,
                ))),
                border: iced::Border { radius: 40.0.into(), ..Default::default() },
                ..Default::default()
            }),
            column![
                text("Privacy & Security").size(26).style(move |_| text::Style {
                    color: Some(theme.text_primary()),
                    ..Default::default()
                }),
                row![
                    container(
                        text(format!("Security Score: {} / 100", score)).size(13).style(move |_| text::Style {
                            color: Some(score_color),
                            ..Default::default()
                        })
                    )
                    .padding([3, 8])
                    .style(move |_| container::Style {
                        background: Some(iced::Background::Color(Color::from_rgba(
                            score_color.r, score_color.g, score_color.b, 0.15,
                        ))),
                        border: iced::Border { radius: 6.0.into(), ..Default::default() },
                        ..Default::default()
                    }),
                    text(score_label).size(13).style(move |_| text::Style {
                        color: Some(score_color),
                        ..Default::default()
                    }),
                ].spacing(8).align_y(Alignment::Center),
            ].spacing(8),
        ]
        .spacing(20)
        .align_y(Alignment::Center)
    )
    .padding(iced::Padding { top: 30.0, right: 40.0, bottom: 20.0, left: 40.0 });

    // ─── Recommendations ──────────────────────────────────────────────────
    let mut recommendations_col = column![].spacing(8);
    for rec in &security.recommendations {
        let rec_color = match rec.severity {
            RecommendationSeverity::High => Color::from_rgb8(255, 59, 48),
            RecommendationSeverity::Medium => Color::from_rgb8(255, 149, 0),
            RecommendationSeverity::Low => Color::from_rgb8(0, 122, 255),
        };

        recommendations_col = recommendations_col.push(
            container(
                row![
                    svg(icons::icon_alert_triangle())
                        .width(16)
                        .height(16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(rec_color) }),
                    column![
                        text(&rec.title).size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        text(&rec.description).size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].spacing(2),
                ].spacing(12).align_y(Alignment::Center)
            )
            .padding([10, 15])
            .width(Length::Fill)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(Color::from_rgba(rec_color.r, rec_color.g, rec_color.b, 0.08))),
                border: iced::Border {
                    color: Color::from_rgba(rec_color.r, rec_color.g, rec_color.b, 0.3),
                    width: 1.0,
                    radius: 8.0.into(),
                },
                ..Default::default()
            })
        );
    }

    let recommendations_section = if security.recommendations.is_empty() {
        column![]
    } else {
        column![
            section_header(theme, icons::icon_shield_check(), "Recommendations"),
            recommendations_col,
        ].spacing(10).padding([0, 40])
    };

    // ─── Security Overview Toggles ─────────────────────────────────────────

    let overview_section = column![
        section_header(theme, icons::icon_shield(), "Security Overview"),
        card(theme, column![
            // SSH toggle
            row![
                svg(icons::icon_terminal()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(0, 122, 255)) }),
                text("SSH Server").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(security.ssh_enabled).on_toggle(|v| Message::ToggleSsh(v)),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Firewall toggle
            row![
                svg(icons::icon_shield()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(52, 199, 89)) }),
                text("Firewall").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(security.firewall_active).on_toggle(|v| Message::ToggleFirewall(v)),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Failed logins alert
            row![
                svg(icons::icon_alert_triangle()).width(16).height(16).style(move |_: &iced::Theme, _| svg::Style {
                    color: Some(if security.failed_logins_count > 5 {
                        Color::from_rgb8(255, 59, 48)
                    } else {
                        Color::from_rgb8(255, 214, 10)
                    })
                }),
                text("Failed Login Attempts").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(format!("{}", security.failed_logins_count)).size(13).style(move |_| text::Style {
                    color: Some(if security.failed_logins_count > 5 { Color::from_rgb8(255, 59, 48) } else { theme.text_secondary() }),
                    ..Default::default()
                }),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Last Update
            row![
                svg(icons::icon_refresh()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(142, 142, 147)) }),
                text("Last System Update").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(&security.last_system_update).size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Screen lock timeout dropdown
            row![
                svg(icons::icon_lock()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(255, 149, 0)) }),
                text("Screen Lock Timeout").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                pick_list(
                    vec!["1 minute".to_string(), "5 minutes".to_string(), "10 minutes".to_string(), "15 minutes".to_string(), "30 minutes".to_string(), "Never".to_string()],
                    Some(app.screen_lock_timeout.clone()),
                    |v| Message::SetScreenLockTimeout(v),
                ).width(150),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Password required after sleep
            row![
                svg(icons::icon_key()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(175, 82, 222)) }),
                column![
                    text("Require Password After Sleep").size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Prompt for password when screen wakes").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(2),
                toggler(app.require_password_sleep).on_toggle(|v| Message::ToggleRequirePassword(v)),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),
        ].spacing(0))
    ].spacing(10).padding([0, 40]);

    // ─── Users & Groups ────────────────────────────────────────────────────
    let mut users_col = column![].spacing(6);
    for user in &security.users {
        let is_root = user.uid == 0;
        let avatar_color = user_color(&user.username);
        let initials = user.username.chars().next().unwrap_or('?').to_uppercase().to_string();

        let avatar = container(
            text(initials).size(16).style(move |_| text::Style {
                color: Some(Color::WHITE),
                ..Default::default()
            })
        )
        .width(38)
        .height(38)
        .center_x(38)
        .center_y(38)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(avatar_color)),
            border: iced::Border { radius: 19.0.into(), ..Default::default() },
            ..Default::default()
        });

        let badges = row(
            std::iter::empty()
                .chain(if user.is_sudoer {
                    vec![badge_widget("sudo", Color::from_rgb8(255, 149, 0))]
                } else { vec![] })
                .chain(if is_root {
                    vec![badge_widget("root", Color::from_rgb8(255, 59, 48))]
                } else { vec![] })
                .chain(if user.groups.contains(&"wheel".to_string()) {
                    vec![badge_widget("wheel", Color::from_rgb8(175, 82, 222))]
                } else { vec![] })
                .collect::<Vec<_>>()
        ).spacing(4);

        let user_row = container(
            row![
                avatar,
                column![
                    row![
                        text(&user.username).size(14).style(move |_| text::Style {
                            color: Some(theme.text_primary()),
                            ..Default::default()
                        }),
                        badges,
                    ].spacing(8).align_y(Alignment::Center),
                    text(format!("UID: {}  •  Shell: {}  •  Home: {}", user.uid, user.shell.rsplit('/').next().unwrap_or(&user.shell), user.home))
                        .size(11)
                        .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(4).width(Length::Fill),
                column![
                    text("Last Login").size(10).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text(&user.last_login).size(11).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                ].spacing(2).align_x(Alignment::End),
            ]
            .spacing(12)
            .align_y(Alignment::Center)
        )
        .padding([10, 15])
        .width(Length::Fill)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(
                if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.03) }
                else { Color::from_rgba(0.0, 0.0, 0.0, 0.02) }
            )),
            border: iced::Border {
                color: theme.border(),
                width: 1.0,
                radius: 10.0.into(),
            },
            ..Default::default()
        });

        users_col = users_col.push(user_row);
    }

    let users_section = column![
        section_header(theme, icons::icon_users(), "Users & Groups"),
        card(theme, users_col)
    ].spacing(10).padding([0, 40]);

    // ─── Login History ─────────────────────────────────────────────────────
    let mut login_col = column![
        container(
            row![
                text("User").size(11).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text("From").size(11).width(Length::Fixed(130.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text("Terminal").size(11).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text("Date").size(11).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text("Status").size(11).width(Length::Fixed(70.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(8)
        ).padding([6, 15])
    ].spacing(1);

    for entry in security.login_history.iter().take(15) {
        let status_color = match entry.status {
            LoginStatus::Success => Color::from_rgb8(52, 199, 89),
            LoginStatus::Failed => Color::from_rgb8(255, 59, 48),
        };
        let status_text = match entry.status {
            LoginStatus::Success => "✓ OK",
            LoginStatus::Failed => "✗ FAIL",
        };

        let row_bg = match entry.status {
            LoginStatus::Failed => Color::from_rgba(1.0, 0.23, 0.19, 0.07),
            LoginStatus::Success => Color::from_rgba(0.0, 0.0, 0.0, 0.0),
        };

        let login_row = container(
            row![
                text(&entry.username).size(12).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(&entry.from).size(12).width(Length::Fixed(130.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text(&entry.terminal).size(12).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text(&entry.date).size(12).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                text(status_text).size(11).width(Length::Fixed(70.0)).style(move |_| text::Style { color: Some(status_color), ..Default::default() }),
            ].spacing(8).align_y(Alignment::Center)
        )
        .padding([8, 15])
        .width(Length::Fill)
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(row_bg)),
            border: iced::Border { radius: 6.0.into(), ..Default::default() },
            ..Default::default()
        });

        login_col = login_col.push(login_row);
    }

    let login_section = column![
        section_header(theme, icons::icon_clock(), "Login & Audit History"),
        card(theme, login_col)
    ].spacing(10).padding([0, 40]);

    // ─── Network & Ports ───────────────────────────────────────────────────
    let ports_content = if security.open_ports.is_empty() {
        column![
            container(
                text("No listening ports detected").size(13).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            ).padding([20, 15])
        ]
    } else {
        let mut ports_col = column![
            container(
                row![
                    text("Port").size(11).width(Length::Fixed(60.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text("Protocol").size(11).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text("Service").size(11).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text("State").size(11).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(10)
            ).padding([6, 15])
        ].spacing(1);

        for port in &security.open_ports {
            let port_color = match port.port {
                22 => Color::from_rgb8(255, 149, 0), // SSH - warn
                80 | 443 => Color::from_rgb8(0, 122, 255), // Web - info
                _ => Color::from_rgb8(142, 142, 147),
            };

            let port_row = container(
                row![
                    text(format!("{}", port.port)).size(12).width(Length::Fixed(60.0)).style(move |_| text::Style { color: Some(port_color), ..Default::default() }),
                    text(&port.protocol).size(12).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text(&port.service).size(12).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    container(
                        text(&port.state).size(11).style(|_| text::Style { color: Some(Color::from_rgb8(52, 199, 89)), ..Default::default() })
                    )
                    .width(Length::Fixed(80.0))
                    .padding([2, 8])
                    .style(|_| container::Style {
                        background: Some(iced::Background::Color(Color::from_rgba(0.2, 0.78, 0.35, 0.15))),
                        border: iced::Border { radius: 4.0.into(), ..Default::default() },
                        ..Default::default()
                    }),
                ].spacing(10).align_y(Alignment::Center)
            )
            .padding([8, 15])
            .width(Length::Fill);

            ports_col = ports_col.push(ports_col_separator(theme, ports_col_item(theme, port_row)));
        }

        ports_col
    };

    let ports_section = column![
        section_header(theme, icons::icon_server(), "Open Ports & Services"),
        card(theme, ports_content)
    ].spacing(10).padding([0, 40]);

    // ─── Advanced Security Settings ────────────────────────────────────────
    let advanced_section = column![
        section_header(theme, icons::icon_key(), "Advanced Security"),
        card(theme, column![
            // Password policy
            row![
                svg(icons::icon_key()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(175, 82, 222)) }),
                text("Password Policy").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                pick_list(
                    vec!["Basic".to_string(), "Strong".to_string(), "Very Strong".to_string()],
                    Some(app.password_policy.clone()),
                    |v| Message::SetPasswordPolicy(v),
                ).width(160),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Session timeout
            row![
                svg(icons::icon_clock()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(0, 122, 255)) }),
                column![
                    text("Login Session Timeout").size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Auto-lock terminal sessions after inactivity").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(2).width(Length::Fill),
                pick_list(
                    vec!["5 minutes".to_string(), "15 minutes".to_string(), "30 minutes".to_string(), "1 hour".to_string(), "Disabled".to_string()],
                    Some(app.session_timeout.clone()),
                    |v| Message::SetSessionTimeout(v),
                ).width(160),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // Audit logging
            row![
                svg(icons::icon_activity()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(52, 199, 89)) }),
                column![
                    text("Audit Logging").size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Log all authentication events and sudo usage").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(2).width(Length::Fill),
                toggler(security.audit_logging_enabled).on_toggle(|v| Message::ToggleAuditLogging(v)),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // USB guard
            row![
                svg(icons::icon_alert_triangle()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(255, 149, 0)) }),
                column![
                    text("USB Device Guard").size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Block unauthorized USB devices").size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(2).width(Length::Fill),
                toggler(app.usb_guard_enabled).on_toggle(|v| Message::ToggleUsbGuard(v)),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),

            container(iced::widget::horizontal_rule(1)).padding([4, 0]),

            // View logs button
            row![
                svg(icons::icon_terminal()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(142, 142, 147)) }),
                text("Auth Log Path").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text("/var/log/auth.log").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(10).align_y(Alignment::Center).padding([8, 5]),
        ].spacing(0))
    ].spacing(10).padding([0, 40]);

    // ─── Assemble Page ─────────────────────────────────────────────────────
    let page_content = column![
        header,
        recommendations_section,
        overview_section,
        users_section,
        login_section,
        ports_section,
        advanced_section,
        container(iced::widget::Space::new(Length::Fill, Length::Fixed(40.0))),
    ]
    .spacing(24)
    .width(Length::Fill);

    container(
        scrollable(page_content).direction(
            iced::widget::scrollable::Direction::Vertical(
                    iced::widget::scrollable::Scrollbar::new().width(0).scroller_width(0).margin(0)
            )
        )
    )
    .width(Length::Fill)
    .height(Length::Fill)
    .into()
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

fn security_score(info: &SecurityInfo) -> u32 {
    let mut score = 100u32;
    if !info.firewall_active { score = score.saturating_sub(25); }
    if info.ssh_enabled { score = score.saturating_sub(10); }
    if info.failed_logins_count > 10 { score = score.saturating_sub(20); }
    else if info.failed_logins_count > 3 { score = score.saturating_sub(10); }
    if info.open_ports.len() > 5 { score = score.saturating_sub(10); }
    score
}

fn section_header<'a>(theme: &'a MacOSTheme, icon: iced::widget::svg::Handle, title: &'static str) -> Element<'a, Message> {
    row![
        svg(icon)
            .width(16)
            .height(16)
            .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
        text(title).size(13).style(move |_| text::Style {
            color: Some(theme.text_secondary()),
            ..Default::default()
        }),
    ]
    .spacing(6)
    .align_y(Alignment::Center)
    .into()
}

fn badge_widget(label: &str, color: Color) -> Element<'_, Message> {
    container(
        text(label).size(10).style(move |_| text::Style { color: Some(color), ..Default::default() })
    )
    .padding([2, 6])
    .style(move |_| container::Style {
        background: Some(iced::Background::Color(Color::from_rgba(color.r, color.g, color.b, 0.18))),
        border: iced::Border { color, width: 0.5, radius: 4.0.into() },
        ..Default::default()
    })
    .into()
}

fn user_color(username: &str) -> Color {
    // Deterministic color from username
    let hash: u32 = username.bytes().fold(0u32, |acc, b| acc.wrapping_mul(31).wrapping_add(b as u32));
    let colors = [
        Color::from_rgb8(0, 122, 255),
        Color::from_rgb8(52, 199, 89),
        Color::from_rgb8(255, 149, 0),
        Color::from_rgb8(175, 82, 222),
        Color::from_rgb8(255, 59, 48),
        Color::from_rgb8(90, 200, 250),
        Color::from_rgb8(255, 214, 10),
    ];
    colors[(hash as usize) % colors.len()]
}

fn ports_col_item<'a>(_theme: &MacOSTheme, item: iced::widget::Container<'a, Message>) -> Element<'a, Message> {
    item.into()
}

fn ports_col_separator<'a>(_theme: &'a MacOSTheme, item: Element<'a, Message>) -> Element<'a, Message> {
    column![
        item,
        container(iced::widget::horizontal_rule(1)).padding([0, 15]),
    ].spacing(0).into()
}

