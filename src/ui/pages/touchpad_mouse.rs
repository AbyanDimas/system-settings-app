use iced::{
    widget::{column, row, text, container, toggler, slider, scrollable, Space},
    Alignment, Element, Length,
};
use crate::app::{App, Message};
use crate::theme::card;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let mouse_section = column![
        container(text("Mouse").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 0.0, right: 0.0, bottom: 5.0, left: 10.0 }),
        card(theme, column![
            column![
                text("Tracking speed").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                row![
                    text("Slow").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    slider(-1.0..=1.0, app.mouse_sensitivity, Message::SetMouseSensitivity).step(0.01).width(Length::Fill),
                    text("Fast").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(15).align_y(Alignment::Center),
            ].spacing(10).padding([10, 15]),
        ].spacing(0)),
    ].spacing(10);

    let touchpad_section = column![
        container(text("Touchpad").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 20.0, right: 0.0, bottom: 5.0, left: 10.0 }),
        card(theme, column![
            row![
                column![
                    text("Tap to click").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Tap with one finger to click").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.touchpad_tap_to_click).on_toggle(Message::ToggleTouchpadTapToClick),
            ].align_y(Alignment::Center).padding([10, 15]),

            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),

            row![
                column![
                    text("Natural scrolling").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Content tracks finger movement").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.touchpad_natural_scroll).on_toggle(Message::ToggleTouchpadNaturalScroll),
            ].align_y(Alignment::Center).padding([10, 15]),

            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),

            column![
                text("Tracking speed").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                row![
                    text("Slow").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    slider(-1.0..=1.0, app.touchpad_sensitivity, Message::SetTouchpadSensitivity).step(0.01).width(Length::Fill),
                    text("Fast").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(15).align_y(Alignment::Center),
            ].spacing(10).padding([15, 15]),

            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),

            column![
                text("Scroll Speed").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                row![
                    text("Slow").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    slider(0.1..=1.5, app.touchpad_scroll_factor, Message::SetTouchpadScrollFactor).step(0.01).width(Length::Fill),
                    text("Fast").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(15).align_y(Alignment::Center),
            ].spacing(10).padding([15, 15]),
        ].spacing(0)),
    ].spacing(10);

    let gesture_section = column![
        container(text("Gestures").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 20.0, right: 0.0, bottom: 5.0, left: 10.0 }),
        card(theme, column![
            row![
                column![
                    text("Swipe between workspaces").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Swipe left or right with three fingers").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.gesture_workspace_swipe).on_toggle(Message::ToggleGestureWorkspaceSwipe),
            ].align_y(Alignment::Center).padding([10, 15]),

            container(iced::widget::horizontal_rule(1)).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 0.0, left: 10.0 }),

            row![
                column![
                    text("Three Finger Drag").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Left-click-and-drag with three fingers").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.touchpad_drag_3fg).on_toggle(Message::ToggleTouchpadDrag3fg),
            ].align_y(Alignment::Center).padding([10, 15]),
        ].spacing(0)),
    ].spacing(10);

    let keyboard_section = column![
        container(text("Keyboard").size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })).padding(iced::Padding { top: 20.0, right: 0.0, bottom: 5.0, left: 10.0 }),
        card(theme, column![
            row![
                column![
                    text("Numlock by default").size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text("Enable numeric keypad at startup").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill),
                toggler(app.numlock_by_default).on_toggle(Message::ToggleNumlockByDefault),
            ].align_y(Alignment::Center).padding([10, 15]),
        ].spacing(0)),
    ].spacing(10);

    let content = column![
        text("Touchpad & Mouse").size(28).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        mouse_section,
        touchpad_section,
        gesture_section,
        keyboard_section,
        Space::with_height(20),
    ]
    .spacing(10)
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
