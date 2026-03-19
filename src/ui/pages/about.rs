use iced::{
    widget::{column, row, text, container, svg, scrollable, button},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message, Route};
use crate::theme::{card, MacOSTheme};
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let content = if let Some(sys) = &app.sys_info {
        // OS Logo/Header area
        let header = column![
            container(
                svg(icons::icon_system()) // Fallback logo
                    .width(64)
                    .height(64)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) })
            )
            .padding(20)
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(
                    if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) }
                    else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                )),
                border: iced::Border { radius: 32.0.into(), ..Default::default() },
                ..Default::default()
            }),
            text(&sys.os)
                .size(32)
                .style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            text(format!("Kernel {}", sys.kernel))
                .size(16)
                .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        ]
        .spacing(15)
        .align_x(Alignment::Center)
        .width(Length::Fill);

        column![
            button(
                row![
                    svg(icons::icon_chevron_right()).width(16).height(16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                    text("Back").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                ].spacing(5).align_y(Alignment::Center)
            )
            .padding([5, 10])
            .on_press(Message::Navigate(Route::System))
            .style(button::text),
            
            header,
            
            container(text("Hardware").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 20.0, right: 0.0, bottom: 5.0, left: 10.0 }),
            card(theme, column![
                info_row(theme, "Device Name", sys.hostname.clone()),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Processor", sys.cpu.clone()),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Memory", sys.memory.clone()),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Storage", format!("{} used of {}", sys.disk_used, sys.disk_total)),
            ].spacing(2)),

            container(text("Software").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 20.0, right: 0.0, bottom: 5.0, left: 10.0 }),
            card(theme, column![
                info_row(theme, "OS Version", sys.os.clone()),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Window Manager", sys.wm.clone()),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Packages", format!("{} (pacman)", sys.packages)),
                container(iced::widget::horizontal_rule(1)).padding([0, 10]),
                info_row(theme, "Uptime", sys.uptime.clone()),
            ].spacing(2)),
        ]
        .spacing(10)
        .padding(iced::Padding { top: 20.0, right: 60.0, bottom: 40.0, left: 60.0 })
        .width(Length::Fill)
    } else {
        column![text("Loading system information...").style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })]
            .padding(40)
            .width(Length::Fill)
    };

    container(
        scrollable(
            container(content).width(Length::Fill)
        ).direction(
            iced::widget::scrollable::Direction::Vertical(
                    iced::widget::scrollable::Scrollbar::new().width(0).scroller_width(0).margin(0)
            )
        )
    )
    .width(Length::Fill)
    .height(Length::Fill)
    .into()
}

fn info_row<'a>(theme: &'a MacOSTheme, label: &'a str, value: String) -> iced::Element<'a, Message> {
    container(
        row![
            text(label).size(14).width(140).style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),
            text(value).size(14).width(Length::Fill).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
        ].align_y(Alignment::Center)
    )
    .padding([12, 15])
    .into()
}
