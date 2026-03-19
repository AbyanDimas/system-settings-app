use iced::{widget::{column, row, text, slider, button, container, svg, scrollable, pick_list}, Element, Alignment, Length};
use crate::app::{App, Message};
use crate::theme::{card, MacOSTheme};
use crate::ui::icons;
use crate::backend::audio::AudioCard;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;
    
    let default_sink = app.audio_sinks.iter().find(|s| s.is_default);
    let default_source = app.audio_sources.iter().find(|s| s.is_default);

    let mut content = column![
        text("Sound").size(24).width(Length::Fill).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
    ].spacing(25);

    // --- Output Section ---
    content = content.push(
        column![
            text("Output").size(16).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            card(theme, column![
                // Device Dropdown
                row![
                    text("Device").size(14).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    pick_list(
                        &app.audio_sinks[..],
                        default_sink.cloned(),
                        |s| Message::SetDefaultSink(s.id)
                    ).width(Length::Fill),
                ].spacing(15).align_y(Alignment::Center),

                container(iced::widget::horizontal_rule(1)).padding(5),

                // Volume Slider
                if let Some(sink) = default_sink {
                    column![row![
                        button(
                            svg(if sink.is_muted { icons::icon_volume_mute() } else { 
                                if sink.volume < 33 { icons::icon_volume_low() } 
                                else if sink.volume < 66 { icons::icon_volume_mid() }
                                else { icons::icon_volume_high() }
                            })
                            .width(18)
                            .height(18)
                            .style(move |_: &iced::Theme, _| svg::Style { 
                                color: Some(if sink.is_muted { theme.text_secondary() } else { theme.accent() }) 
                            })
                        )
                        .on_press(Message::ToggleMute)
                        .style(button::text),
                        
                        slider(0..=100, sink.volume, Message::SetVolume)
                            .width(Length::Fill),
                        
                        text(format!("{}%", sink.volume))
                            .width(40)
                            .size(13)
                            .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].spacing(15).align_y(Alignment::Center)]
                } else {
                    column![row![text("No output device found").size(13).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })]]
                }
            ].spacing(15))
        ].spacing(10)
    );

    // --- Input Section ---
    content = content.push(
        column![
            text("Input").size(16).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            card(theme, column![
                // Device Dropdown
                row![
                    text("Device").size(14).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    pick_list(
                        &app.audio_sources[..],
                        default_source.cloned(),
                        |s| Message::SetDefaultSource(s.id)
                    ).width(Length::Fill),
                ].spacing(15).align_y(Alignment::Center),

                container(iced::widget::horizontal_rule(1)).padding(5),

                // Volume Slider & Mic Test
                if let Some(source) = default_source {
                    column![
                        row![
                            button(
                                svg(if source.is_muted { icons::icon_mic_mute() } else { icons::icon_mic() })
                                    .width(18)
                                    .height(18)
                                    .style(move |_: &iced::Theme, _| svg::Style { 
                                        color: Some(if source.is_muted { theme.text_secondary() } else { theme.accent() }) 
                                    })
                            )
                            .on_press(Message::ToggleDeviceMute(source.id.clone(), false))
                            .style(button::text),
                            
                            slider(0..=100, source.volume, move |v| Message::SetSourceVolume(source.id.clone(), v))
                                .width(Length::Fill),
                            
                            text(format!("{}%", source.volume))
                                .width(40)
                                .size(13)
                                .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                        ].spacing(15).align_y(Alignment::Center),

                        // Input Level (Mic Test)
                        row![
                            text("Input Level").size(12).width(Length::Fixed(100.0)).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                            container(
                                row((0..20).map(|i| {
                                    let active = !source.is_muted && (i as f32 / 20.0) < (source.volume as f32 / 100.0);
                                    container(text(""))
                                        .width(Length::Fill)
                                        .height(4)
                                        .style(move |_| container::Style {
                                            background: Some(iced::Background::Color(if active { 
                                                if i < 15 { theme.accent() } else { iced::Color::from_rgb(1.0, 0.4, 0.4) }
                                            } else { 
                                                if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                                            })),
                                            border: iced::Border { radius: 1.0.into(), ..Default::default() },
                                            ..Default::default()
                                        })
                                        .into()
                                }).collect::<Vec<_>>()).spacing(2)
                            ).width(Length::Fill).height(8)
                        ].spacing(15).align_y(Alignment::Center)
                    ].spacing(15)
                } else {
                    column![row![text("No input device found").size(13).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })]]
                }
            ].spacing(15))
        ].spacing(10)
    );

    // Wrap everything in a scrollable container
    container(
        scrollable(content.padding([40, 60]))
            .direction(iced::widget::scrollable::Direction::Vertical(
                iced::widget::scrollable::Scrollbar::new()
                    .width(4)
                    .scroller_width(4)
                    .margin(2)
            ))
    )
    .width(Length::Fill)
    .height(Length::Fill)
    .into()
}

fn card_profile_view<'a>(theme: &'a MacOSTheme, audio_card: &'a AudioCard) -> Element<'a, Message> {
    card(theme, column![
        text(&audio_card.description).size(14).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        pick_list(
            &audio_card.profiles[..],
            audio_card.profiles.iter().find(|p| p.name == audio_card.active_profile).cloned(),
            move |p| Message::SetCardProfile(audio_card.id.clone(), p.name)
        ).width(Length::Fill),
    ].spacing(10)).into()
}

