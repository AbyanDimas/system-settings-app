use iced::{
    widget::{column, row, text, container, toggler, scrollable, button, svg},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

fn format_secs(secs: u64) -> String {
    let mins = secs / 60;
    let s = secs % 60;
    format!("{:02}:{:02}", mins, s)
}

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;
    let focus = &app.focus_state;

    let status_card = card(theme, column![
        row![
            container(
                svg(icons::icon_moon())
                    .width(24)
                    .height(24)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(if focus.active { Color::from_rgb8(88, 86, 214) } else { theme.text_secondary() }) })
            )
            .padding(10)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                border: iced::Border { radius: 10.0.into(), ..Default::default() },
                ..Default::default()
            }),
            column![
                text("Focus Mode").size(16).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(if focus.active { 
                    if !focus.tag.is_empty() { format!("Active: {}", focus.tag) } else { "Active".to_string() }
                } else { 
                    "Inactive".to_string() 
                }).size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].width(Length::Fill).spacing(2),
            toggler(focus.active).on_toggle(|active| Message::ToggleFocusMode(active, None)),
        ].spacing(15).align_y(Alignment::Center).padding([10, 15]),

        if let Some(rem) = focus.remaining_secs {
            column![
                container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
                row![
                    text("Time Remaining").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text(format_secs(rem)).size(18).style(move |_| text::Style { color: Some(theme.accent()), ..Default::default() }),
                ].align_y(Alignment::Center).padding([15, 15]),
            ]
        } else {
            column![]
        }
    ].spacing(0));

    let presets = vec![
        ("Study", 25, icons::icon_book()),
        ("Deep Work", 50, icons::icon_brain()),
        ("Break", 5, icons::icon_coffee()),
        ("Long Break", 15, icons::icon_coffee()),
    ];

    let mut preset_row = row![].spacing(15);
    for (name, mins, icon_handle) in presets {
        preset_row = preset_row.push(
            button(
                column![
                    svg(icon_handle).width(24).height(24).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_primary()) }),
                    text(name).size(12),
                    text(format!("{}m", mins)).size(10).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(8).align_x(Alignment::Center)
            )
            .width(Length::Fill)
            .padding(15)
            .on_press(Message::StartFocusTimer(mins, Some(name.to_string())))
            .style(move |_, status| {
                let mut s = button::Style::default();
                s.background = Some(iced::Background::Color(
                    if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } 
                    else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                ));
                if status == button::Status::Hovered {
                    s.background = Some(iced::Background::Color(
                        if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } 
                        else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) }
                    ));
                }
                s.border.radius = 12.0.into();
                s.text_color = theme.text_primary();
                s
            })
        );
    }

    let content = column![
        text("Focus").size(28).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),

        status_card,

        text("Quick Presets").size(18).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
        preset_row,

        text("Integration").size(18).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
        card(theme, column![
            row![
                column![
                    text("Waybar Status").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Focus mode status is shown in your top bar").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                text("Connected").size(12).style(move |_| text::Style { color: Some(Color::from_rgb8(52, 199, 89)), ..Default::default() }),
            ].align_y(Alignment::Center).padding([10, 15]),
        ])
    ]
    .spacing(25)
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
