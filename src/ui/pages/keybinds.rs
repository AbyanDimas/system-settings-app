use iced::{
    widget::{column, row, text, container, scrollable, slider, toggler, button},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // --- Keyboard Brightness Section ---
    let brightness_settings = card(theme, column![
        row![
            text("Adjust keyboard brightness in low light").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            toggler(app.keyboard_brightness > 0).on_toggle(|b| Message::SetKeyboardBrightness(if b { 50 } else { 0 }))
        ].align_y(Alignment::Center).padding([10, 15]),
        
        container(iced::widget::horizontal_rule(1)).padding([0, 15]),
        
        row![
            text("Keyboard brightness").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            slider(0..=100, app.keyboard_brightness, Message::SetKeyboardBrightness).width(150),
        ].align_y(Alignment::Center).padding([10, 15]),
    ].spacing(0));

    // --- Key Repeat Section ---
    let repeat_settings = card(theme, column![
        row![
            text("Key Repeat Rate").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            row![
                text("Off").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                slider(0..=100, app.key_repeat_rate, Message::SetKeyRepeatRate).width(120),
                text("Fast").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(10).align_y(Alignment::Center)
        ].align_y(Alignment::Center).padding([10, 15]),
        
        container(iced::widget::horizontal_rule(1)).padding([0, 15]),
        
        row![
            text("Delay Until Repeat").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            row![
                text("Long").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                slider(100..=1000, app.key_repeat_delay, Message::SetKeyRepeatDelay).width(120),
                text("Short").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].spacing(10).align_y(Alignment::Center)
        ].align_y(Alignment::Center).padding([10, 15]),
    ].spacing(0));

    // --- Keyboard Navigation & Modifiers ---
    let nav_settings = card(theme, column![
        row![
            text("Swap Ctrl and Alt (Modifier keys)").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            toggler(app.swap_ctrl_alt).on_toggle(Message::ToggleSwapCtrlAlt)
        ].align_y(Alignment::Center).padding([10, 15]),

        container(iced::widget::horizontal_rule(1)).padding([0, 15]),

        column![
            row![
                text("Keyboard navigation").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(app.keyboard_navigation).on_toggle(Message::ToggleKeyboardNavigation)
            ].align_y(Alignment::Center),
            text("Use keyboard navigation to move focus between controls. Press the Tab key to move focus forward and Shift Tab to move focus backward.")
                .size(12)
                .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
        ].spacing(5).padding([10, 15]),
        
        container(
            row![
                button(text("Modifier Keys...").size(13)).padding([6, 12]).style(button::secondary),
                button(text("Keyboard Shortcuts...").size(13)).padding([6, 12]).style(button::secondary),
            ].spacing(10)
        ).width(Length::Fill).align_x(Alignment::End).padding(iced::Padding { top: 5.0, right: 15.0, bottom: 15.0, left: 15.0 })
    ].spacing(0));

    // --- Shortcuts Section (List) ---
    let mut binds_col = column![];
    let len = app.keybinds.len();
    
    for (i, bind) in app.keybinds.iter().enumerate() {
        let content = row![
            column![
                text(&bind.description).size(14).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(format!("{} {}", bind.dispatcher, bind.arg)).size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].width(Length::Fill).spacing(2),
            
            // Format the key binding visually like a physical keyboard key
            container(
                text(&bind.key).size(12).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() })
            )
            .padding([4, 8])
            .style(move |_| container::Style {
                background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                border: iced::Border { radius: 6.0.into(), ..Default::default() },
                ..Default::default()
            })
        ].align_y(Alignment::Center).padding([10, 15]);

        binds_col = binds_col.push(content);

        if i < len - 1 {
            binds_col = binds_col.push(
                container(iced::widget::horizontal_rule(1)).padding([0, 15])
            );
        }
    }

    let shortcuts_card = card(theme, binds_col.spacing(0));

    let content = column![
        text("Keyboard").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        
        brightness_settings,
        repeat_settings,
        nav_settings,
        
        text("Keyboard Shortcuts").size(16).style(move |_| text::Style {
            color: Some(theme.text_secondary()),
            ..Default::default()
        }),
        
        shortcuts_card,
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
