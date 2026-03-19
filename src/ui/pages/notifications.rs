use iced::{
    widget::{column, row, text, container, toggler, scrollable, button, text_input, slider, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;
    let mako = &app.mako_config;

    let content = column![
        text("Notifications").size(28).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),

        card(theme, column![
            row![
                column![
                    text("Do Not Disturb").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Silence all notifications").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.focus_state.active).on_toggle(|active| Message::ToggleFocusMode(active, None)),
            ].align_y(Alignment::Center).padding([10, 15]),
            
            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),
            
            row![
                column![
                    text("Test Notification").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Send a sample notification to check style").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                button(text("Send Test").size(12))
                    .padding([6, 12])
                    .on_press(Message::SendTestNotification)
                    .style(button::secondary),
            ].align_y(Alignment::Center).padding([10, 15]),
        ].spacing(0)),

        text("Mako Customization").size(18).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),

        card(theme, column![
            row![
                text("Background Color").size(14).width(Length::Fixed(150.0)).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text_input("e.g. #00000080", &mako.background_color)
                    .on_input(|v| {
                        let mut new_mako = mako.clone();
                        new_mako.background_color = v;
                        Message::UpdateMakoConfig(new_mako)
                    })
                    .width(Length::Fill),
            ].align_y(Alignment::Center).padding([10, 15]),

            row![
                text("Border Color").size(14).width(Length::Fixed(150.0)).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text_input("e.g. #FFFFFF", &mako.border_color)
                    .on_input(|v| {
                        let mut new_mako = mako.clone();
                        new_mako.border_color = v;
                        Message::UpdateMakoConfig(new_mako)
                    })
                    .width(Length::Fill),
            ].align_y(Alignment::Center).padding([10, 15]),

            row![
                text("Border Radius").size(14).width(Length::Fixed(150.0)).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                slider(0..=30, mako.border_radius, |v| {
                    let mut new_mako = mako.clone();
                    new_mako.border_radius = v;
                    Message::UpdateMakoConfig(new_mako)
                }),
                text(format!("{}", mako.border_radius)).size(12).width(Length::Fixed(30.0)),
            ].align_y(Alignment::Center).padding([10, 15]),

            row![
                text("Padding").size(14).width(Length::Fixed(150.0)).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                slider(0..=50, mako.padding, |v| {
                    let mut new_mako = mako.clone();
                    new_mako.padding = v;
                    Message::UpdateMakoConfig(new_mako)
                }),
                text(format!("{}", mako.padding)).size(12).width(Length::Fixed(30.0)),
            ].align_y(Alignment::Center).padding([10, 15]),
            
            row![
                Space::with_width(Length::Fill),
                button(text("Save & Apply").size(13))
                    .padding([8, 20])
                    .on_press(Message::SaveMakoConfig)
                    .style(move |_, _| {
                        let mut s = button::Style::default();
                        s.background = Some(iced::Background::Color(theme.accent()));
                        s.text_color = Color::WHITE;
                        s.border.radius = 8.0.into();
                        s
                    })
            ].padding([10, 15])
        ].spacing(0)),
    ].spacing(25).padding([40, 60]).width(Length::Fill);

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
