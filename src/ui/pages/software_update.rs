use iced::{
    widget::{column, row, text, container, svg, scrollable, button},
    Alignment, Element, Length,
};
use crate::app::{App, Message, Route};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let content = column![
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

        text("Software Update").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),

        container(
            column![
                svg(icons::icon_refresh())
                    .width(48)
                    .height(48)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }),
                text("Your system is up to date.").size(16).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(format!("Last checked: {}", app.security_info.last_system_update)).size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ]
            .spacing(10)
            .align_x(Alignment::Center)
        )
        .width(Length::Fill)
        .padding(40),

        card(theme, column![
            row![
                column![
                    text("Automatic Updates").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Keep your system updated automatically.").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                text("Off").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                svg(icons::icon_chevron_right())
                    .width(16)
                    .height(16)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
            ].spacing(10).align_y(Alignment::Center).padding([10, 15]),
        ].spacing(0)),
    ]
    .spacing(20)
    .padding(iced::Padding { top: 20.0, right: 60.0, bottom: 40.0, left: 60.0 })
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
