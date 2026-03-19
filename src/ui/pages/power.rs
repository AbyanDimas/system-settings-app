use iced::{widget::{column, row, text, container, button, svg, scrollable, stack, slider}, Alignment, Element, Length, Color};
use crate::app::{App, Message};
use crate::theme::{card, MacOSTheme};
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // Solid colors, no pulse
    let (fill_color, bg_color) = if app.is_charging {
        (Color::from_rgb8(52, 199, 89), Color::from_rgb8(210, 240, 220)) // Mac Green
    } else if app.battery_percent < 20 {
        (Color::from_rgb8(255, 59, 48), Color::from_rgb8(255, 220, 215)) // Mac Red
    } else if app.battery_percent < 50 {
        (Color::from_rgb8(255, 149, 0), Color::from_rgb8(255, 235, 210)) // Mac Orange
    } else {
        (Color::from_rgb8(0, 122, 255), Color::from_rgb8(220, 235, 255)) // Mac Blue
    };

    let fill_portion = app.battery_percent.max(1) as u16;

    // Seamless progress bar using overlapping containers
    let progress_bar = stack![
        // Background track (full width)
        container(text(""))
            .width(Length::Fill)
            .height(Length::Fill)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(bg_color)),
                border: iced::Border { radius: 24.0.into(), ..Default::default() },
                ..Default::default()
            }),
            
        // Fill track
        row![
            container(text(""))
                .width(Length::FillPortion(fill_portion))
                .height(Length::Fill)
                .style(move |_| container::Style {
                    background: Some(iced::Background::Color(fill_color)),
                    border: iced::Border { radius: 24.0.into(), ..Default::default() },
                    ..Default::default()
                }),
            container(text("")).width(Length::FillPortion(100 - fill_portion))
        ].width(Length::Fill).height(Length::Fill).spacing(0)
    ];

    let banner_content = container(
        column![
            text(format!("{}%", app.battery_percent))
                .size(64)
                .style(move |_| text::Style { color: Some(Color::WHITE), ..Default::default() }),
            text(if app.is_charging { format!("Charging • {}", app.battery_remaining_time) } else { format!("{} remaining", app.battery_remaining_time) })
                .size(16)
                .style(move |_| text::Style { color: Some(Color::WHITE), ..Default::default() }),
        ].spacing(5)
    )
    .padding([30, 40])
    .width(Length::Fill)
    .height(Length::Fill);

    let battery_status_banner = container(
        stack![
            progress_bar,
            banner_content
        ]
    )
    .width(Length::Fill)
    .height(Length::Fixed(160.0))
    .style(move |_| container::Style {
        border: iced::Border { radius: 24.0.into(), ..Default::default() },
        shadow: iced::Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.15),
            offset: iced::Vector::new(0.0, 4.0),
            blur_radius: 10.0,
        },
        ..Default::default()
    });

    let display_sleep_section = card(theme, column![
        timeout_slider_row(
            theme, 
            "Turn display off after", 
            app.hypridle_config.dpms_timeout, 
            60..=3600, // 1 min to 60 mins
            Message::SetDpmsTimeout
        ),
        container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
        timeout_slider_row(
            theme, 
            "Lock screen after", 
            app.hypridle_config.lock_timeout, 
            60..=3600,
            Message::SetLockTimeout
        ),
        container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
        timeout_slider_row(
            theme, 
            "Suspend after", 
            app.hypridle_config.suspend_timeout, 
            300..=7200, // 5 mins to 2 hours
            Message::SetSuspendTimeout
        ),
        container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
        timeout_slider_row(
            theme, 
            "Hibernate after", 
            app.hypridle_config.hibernate_timeout, 
            1800..=14400, // 30 mins to 4 hours
            Message::SetHibernateTimeout
        ),
    ].spacing(2));

    let content = column![
        text("Battery & Power").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),

        battery_status_banner,

        text("Power Mode").size(16).style(move |_| text::Style {
            color: Some(theme.text_secondary()),
            ..Default::default()
        }),

        card(theme, column![
            power_mode_row(theme, "Performance", "High power usage, maximum speed", app.power_mode == "performance", Message::SetPowerMode("performance".into())),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            power_mode_row(theme, "Balanced", "Standard power usage and performance", app.power_mode == "balanced", Message::SetPowerMode("balanced".into())),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            power_mode_row(theme, "Power Saver", "Reduced power usage, longer battery life", app.power_mode == "power-saver", Message::SetPowerMode("power-saver".into())),
        ].spacing(2)),

        text("Display & Sleep").size(16).style(move |_| text::Style {
            color: Some(theme.text_secondary()),
            ..Default::default()
        }),

        display_sleep_section,

        text("Hardware Details").size(16).style(move |_| text::Style {
            color: Some(theme.text_secondary()),
            ..Default::default()
        }),

        card(theme, column![
            detail_row(theme, "Manufacturer", app.battery_vendor.clone()),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Device Name", app.battery_model.clone()),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Condition (Capacity)", format!("{:.1}%", app.battery_health)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Cycle Count", format!("{}", app.battery_cycle_count)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Voltage (Current)", format!("{:.3} V", app.battery_voltage)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Voltage (Min Design)", format!("{:.3} V", app.battery_voltage_min_design)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Current Power Draw", format!("{:.2} W", app.battery_power_usage)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Full Charge Energy", format!("{:.2} Wh", app.battery_energy_full)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Design Full Energy", format!("{:.2} Wh", app.battery_energy_full_design)),
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            detail_row(theme, "Chemistry", app.battery_technology.clone()),
        ].spacing(2)),
    ]
    .spacing(25)
    .padding([40, 60])
    .width(Length::Fill)
    .height(Length::Shrink); // Fix scrollable panic by shrinking

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

fn format_duration(seconds: u32) -> String {
    if seconds == 0 {
        return "Never".to_string();
    }
    
    let hours = seconds / 3600;
    let minutes = (seconds % 3600) / 60;

    let mut parts = Vec::new();
    if hours > 0 {
        parts.push(format!("{} hr", hours));
    }
    if minutes > 0 {
        parts.push(format!("{} min", minutes));
    }

    if parts.is_empty() {
        format!("{} sec", seconds)
    } else {
        parts.join(" ")
    }
}

fn timeout_slider_row<'a>(
    theme: &'a MacOSTheme,
    label: &'a str,
    value: u32,
    range: std::ops::RangeInclusive<u32>,
    msg: fn(u32) -> Message,
) -> Element<'a, Message> {
    container(
        row![
            text(label).size(14).width(Length::Fixed(180.0)).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
            slider(range, value, msg).width(Length::Fill),
            text(format_duration(value)).size(13).width(Length::Fixed(80.0)).style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),
        ].align_y(Alignment::Center).spacing(15)
    )
    .padding([15, 15])
    .into()
}

fn power_mode_row<'a>(
    theme: &'a MacOSTheme,
    label: &'a str,
    desc: &'a str,
    is_active: bool,
    msg: Message
) -> Element<'a, Message> {
    button(
        row![
            column![
                text(label).size(14).style(move |_| text::Style {
                    color: Some(theme.text_primary()),
                    ..Default::default()
                }),
                text(desc).size(12).style(move |_| text::Style {
                    color: Some(theme.text_secondary()),
                    ..Default::default()
                }),
            ].width(Length::Fill),
            if is_active {
                Element::from(
                    svg(icons::icon_check())
                        .width(16)
                        .height(16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) })
                )
            } else {
                Element::from(
                    container(column![])
                        .width(16)
                        .height(16)
                )
            }

        ].align_y(Alignment::Center)
    )
    .padding([10, 15])
    .on_press(msg)
    .style(move |_, status| {
        let mut s = button::Style::default();
        s.background = match status {
            button::Status::Hovered => Some(iced::Background::Color(
                if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) }
                else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
            )),
            _ => None,
        };
        s.border.radius = 8.0.into();
        s
    })
    .width(Length::Fill)
    .into()
}

fn detail_row<'a>(
    theme: &'a MacOSTheme,
    label: &'a str,
    value: String,
) -> Element<'a, Message> {
    container(
        row![
            text(label).size(14).width(Length::Fill).style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),
            text(value).size(14).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
        ].align_y(Alignment::Center)
    )
    .padding([10, 15])
    .into()
}
