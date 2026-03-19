use iced::{
    widget::{column, row, text, container, scrollable, svg, button, image, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

fn format_duration(secs: u64) -> String {
    let hours = secs / 3600;
    let mins = (secs % 3600) / 60;
    if hours > 0 {
        format!("{}h {}m", hours, mins)
    } else if mins > 0 {
        format!("{}m", mins)
    } else {
        "< 1m".to_string()
    }
}

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // --- Graph Section ---
    let max_secs = app.screentime_weekly.iter().map(|d| d.total_secs).max().unwrap_or(3600).max(1);
    let selected_secs = app.screentime_weekly.get(app.screentime_selected_day_idx).map(|d| d.total_secs).unwrap_or(0);
    
    let mut chart_row = row![].spacing(15).align_y(Alignment::End);
    for (i, day) in app.screentime_weekly.iter().enumerate() {
        let h = (day.total_secs as f32 / max_secs as f32) * 120.0; // Max height 120
        let is_selected = i == app.screentime_selected_day_idx;
        
        let bar_color = if is_selected {
            theme.accent() // Active color for selected day
        } else {
            if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.2) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.2) }
        };

        let bar_btn = button(
            container(Space::new(Length::Fixed(32.0), Length::Fixed(h.max(4.0))))
                .style(move |_| container::Style {
                    background: Some(iced::Background::Color(bar_color)),
                    border: iced::Border { radius: iced::border::Radius { top_left: 4.0, top_right: 4.0, bottom_right: 0.0, bottom_left: 0.0 }, ..Default::default() },
                    ..Default::default()
                })
        )
        .padding(0)
        .style(button::text)
        .on_press(Message::SelectScreentimeDay(i));

        chart_row = chart_row.push(
            column![
                bar_btn,
                text(&day.day_name).size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            ].spacing(8).align_x(Alignment::Center)
        );
    }

    let graph_card = container(
        column![
            row![
                column![
                    text("Usage").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    text(format_duration(selected_secs)).size(28).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                ].spacing(2),
                container(
                    row![
                        button(text("<").size(12)).padding([4, 8]).style(button::secondary).on_press(Message::NavigateScreentimeWeek(1)),
                        button(text("Today").size(12)).padding([4, 12]).style(button::secondary).on_press(Message::NavigateScreentimeWeek(-app.screentime_week_offset)),
                        button(text(">").size(12)).padding([4, 8]).style(button::secondary).on_press(Message::NavigateScreentimeWeek(-1)),
                    ].spacing(5)
                ).align_x(Alignment::End).width(Length::Fill)
            ].align_y(Alignment::Center).width(Length::Fill),
            
            container(chart_row).padding(iced::Padding { top: 30.0, right: 0.0, bottom: 10.0, left: 0.0 }).center_x(Length::Fill),
        ].spacing(10)
    )
    .padding(25)
    .style(move |_| container::Style {
        background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.03) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.02) })),
        border: iced::Border { radius: 12.0.into(), color: theme.border(), width: 1.0 },
        ..Default::default()
    });

    // --- App Usage Category Section ---
    let mut apps_col = column![].spacing(0);
    
    // Header for the list
    apps_col = apps_col.push(
        row![
            text("Apps").size(12).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            text("Time").size(12).width(Length::Fixed(80.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ].padding([10, 15])
    );
    apps_col = apps_col.push(container(iced::widget::horizontal_rule(1)).padding([0, 0]));

    if app.screentime_apps.is_empty() {
        apps_col = apps_col.push(
            container(text("No activity for this day").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }))
                .padding(30)
                .center_x(Length::Fill)
        );
    } else {
        for (i, app_usage) in app.screentime_apps.iter().enumerate() {
            let is_last = i == app.screentime_apps.len() - 1;
            
            let icon_widget: Element<'_, Message> = if let Some(path) = &app_usage.icon_path {
                if path.ends_with(".svg") {
                    svg(iced::widget::svg::Handle::from_path(path)).width(24).height(24).into()
                } else {
                    image(path).width(24).height(24).into()
                }
            } else {
                svg(icons::icon_edit()).width(18).height(18).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_primary()) }).into()
            };

            apps_col = apps_col.push(
                row![
                    container(icon_widget)
                        .width(36).height(36).center_x(36).center_y(36)
                        .style(move |_| container::Style {
                            background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                            border: iced::Border { radius: 8.0.into(), ..Default::default() },
                            ..Default::default()
                        }),
                    text(&app_usage.name).size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text(format_duration(app_usage.duration_secs)).size(14).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].align_y(Alignment::Center).spacing(15).padding([10, 15])
            );

            if !is_last {
                apps_col = apps_col.push(
                    container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 0.0, bottom: 0.0, left: 55.0 })
                );
            }
        }
    }

    let now = chrono::Local::now();
    let updated_text = format!("Updated today at {}", now.format("%I:%M %p"));

    let content = column![
        row![
            container(
                svg(icons::icon_hourglass())
                    .width(28)
                    .height(28)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) })
            )
            .padding(10)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                border: iced::Border { radius: 10.0.into(), ..Default::default() },
                ..Default::default()
            }),
            column![
                text("App & Website Activity").size(24).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(updated_text).size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(2),
        ].spacing(15).align_y(Alignment::Center),

        graph_card,

        card(theme, apps_col),
    ]
    .spacing(25)
    .padding([40, 60])
    .width(Length::Fill)
    .height(Length::Shrink);

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
