use iced::{
    widget::{button, column, container, row, text, svg, scrollable, Space, toggler},
    Alignment, Element, Length, Color,
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
                text("General").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            ].spacing(5).align_y(Alignment::Center)
        )
        .padding([5, 0])
        .on_press(Message::Navigate(Route::System))
        .style(button::text),

        text("Game Mode").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),

        container(
            row![
                svg(icons::icon_gamepad()).width(32).height(32)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb8(52, 199, 89)) }),
                column![
                    text("Game Mode").size(16).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Optimizes your system for gaming by prioritizing game processes and reducing background activity.").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill).spacing(2),
                toggler(true).on_toggle(|_| Message::Tick(())), // Placeholder for now
            ]
            .spacing(15)
            .align_y(Alignment::Center)
            .padding([15, 20])
        )
        .style(move |_| container::Style {
            background: Some(theme.card_bg().into()),
            border: iced::Border { radius: 10.0.into(), color: theme.border(), width: 0.5 },
            ..Default::default()
        }),

        text("Performance").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),

        card(theme, column![
            row![
                text("Prioritize Game Processes").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(true).on_toggle(|_| Message::Tick(())),
            ].padding([10, 15]),
            container(iced::widget::horizontal_rule(1)).padding([0, 15]),
            row![
                text("Disable Background Notifications").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(true).on_toggle(|_| Message::Tick(())),
            ].padding([10, 15]),
            container(iced::widget::horizontal_rule(1)).padding([0, 15]),
            row![
                text("Optimize CPU/GPU Governor").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(true).on_toggle(|_| Message::Tick(())),
            ].padding([10, 15]),
        ]),

        text("Automatic Detection").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),

        card(theme, column![
            row![
                text("Auto-detect Steam Games").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(true).on_toggle(|_| Message::Tick(())),
            ].padding([10, 15]),
            container(iced::widget::horizontal_rule(1)).padding([0, 15]),
            row![
                text("Auto-detect Lutris/Heroic").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(true).on_toggle(|_| Message::Tick(())),
            ].padding([10, 15]),
        ]),

        Space::with_height(20),
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
