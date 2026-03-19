use iced::{
    widget::{column, row, text, slider, container, button, scrollable, pick_list, toggler, svg, stack},
    Alignment, Element, Length, Color,
};
use std::fs;
use crate::app::{App, Message};
use crate::theme::{card, MacOSTheme};
use crate::backend::hyprland::AppMonitor;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    if app.show_display_advanced_modal {
        return view_advanced_modal(app, theme);
    }
    if app.show_night_shift_modal {
        return view_night_shift_modal(app, theme);
    }
    
    // --- Monitor Visualizer (macOS Style) ---
    let mut visualizer_row = row![].spacing(40).align_y(Alignment::Center);
    // Canonicalize the symlink so we get the physical file
    let wallpaper_path_str = format!("{}/.config/omarchy/current/background", std::env::var("HOME").unwrap_or_default());
    let wallpaper_path = fs::canonicalize(&wallpaper_path_str)
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or(wallpaper_path_str);
    
    for mon in &app.monitors {
        let is_portrait = mon.transform % 2 != 0;
        let vis_width = if is_portrait { 80.0 } else { 140.0 };
        let vis_height = if is_portrait { 140.0 } else { 80.0 };

        let monitor_screen = container(
            stack![
                iced::widget::image(wallpaper_path.clone())
                    .content_fit(iced::ContentFit::Cover)
                    .width(Length::Fill)
                    .height(Length::Fill),
                container(
                    text(&mon.model)
                        .size(10)
                        .style(|_| text::Style { color: Some(Color::WHITE), ..Default::default() })
                )
                .width(Length::Fill)
                .height(Length::Fill)
                .center_x(Length::Fill)
                .center_y(Length::Fill)
                .style(|_| container::Style {
                    background: Some(iced::Background::Color(Color::from_rgba(0.0, 0.0, 0.0, 0.3))),
                    ..Default::default()
                })
            ]
        )
        .width(Length::Fixed(vis_width))
        .height(Length::Fixed(vis_height))
        .style(move |_| container::Style {
            border: iced::Border {
                color: Color::from_rgb(0.1, 0.1, 0.1),
                width: 6.0,
                radius: 10.0.into(),
            },
            shadow: iced::Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.4),
                offset: iced::Vector::new(0.0, 6.0),
                blur_radius: 12.0,
            },
            ..Default::default()
        });

        visualizer_row = visualizer_row.push(
            column![
                monitor_screen,
                container(
                    text(if mon.id == 0 { "Main Display" } else { "Extended" })
                        .size(10)
                        .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                )
            ].spacing(12).align_x(Alignment::Center)
        );
    }

    let visualizer_area = container(
        column![
            visualizer_row,
            button(
                row![
                    svg(icons::icon_refresh()).width(14).height(14),
                    text("Arrange...").size(12)
                ].spacing(8).align_y(Alignment::Center)
            )
            .padding([8, 16])
            .style(button::secondary)
        ].spacing(30).align_x(Alignment::Center).width(Length::Fill)
    )
    .width(Length::Fill)
    .padding(50)
    .style(move |_| container::Style {
        background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(0.0, 0.0, 0.0, 0.3) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
        border: iced::Border {
            radius: 12.0.into(),
            ..Default::default()
        },
        ..Default::default()
    });

    // --- Monitor Settings ---
    let mut monitors_col = column![].spacing(30).height(Length::Shrink);
    for mon in &app.monitors {
        monitors_col = monitors_col.push(monitor_settings_view(theme, mon, app));
    }

    let content = column![
        text("Displays").size(28).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        
        visualizer_area,
        monitors_col,
    ]
    .spacing(25)
    .padding(iced::Padding { top: 0.0, right: 40.0, bottom: 40.0, left: 40.0 })
    .width(Length::Fill)
    .height(Length::Shrink);

    container(
        scrollable(content)
            .height(Length::Fill)
            .direction(
                iced::widget::scrollable::Direction::Vertical(
                    iced::widget::scrollable::Scrollbar::new().width(0).scroller_width(0).margin(0)
                )
            )
    )
    .width(Length::Fill)
    .height(Length::Fill)
    .into()
}

fn monitor_settings_view<'a>(theme: &'a MacOSTheme, mon: &'a AppMonitor, app: &'a App) -> Element<'a, Message> {
    let name = mon.name.clone();
    
    // Parse resolution and refresh from available_modes
    let mut resolutions: Vec<String> = Vec::new();
    let mut refresh_rates: Vec<String> = Vec::new();
    
    for mode in &mon.available_modes {
        if let Some((res, refresh)) = mode.split_once('@') {
            if !resolutions.contains(&res.to_string()) {
                resolutions.push(res.to_string());
            }
            let rate = refresh.trim_end_matches("Hz");
            if !refresh_rates.contains(&rate.to_string()) {
                refresh_rates.push(rate.to_string());
            }
        }
    }

    let current_res = format!("{}x{}", mon.width, mon.height);
    let current_refresh = format!("{:.2}", mon.refresh_rate);

    // macOS Style Scaling Icons
    // Using None for "Auto" scaling
    let scaling_options = vec![
        ("Larger Text", 10.0, Some(1.5)),
        ("Standard", 14.0, Some(1.25)),
        ("Default", 18.0, Some(1.0)),
        ("Auto", 22.0, None),
    ];

    let scaling_row = row(scaling_options.into_iter().map(|(label, icon_size, val)| {
        let is_active = match val {
            Some(v) => (mon.scale - v).abs() < 0.01,
            None => {
                // If it's not one of the standard scales, we might consider it "Auto" for display purposes
                let is_standard = [1.5, 1.25, 1.0].iter().any(|&s| (mon.scale - s).abs() < 0.01);
                !is_standard
            },
        };
        let mon_name = name.clone();
        
        button(
            column![
                container(
                    svg(icons::icon_display())
                        .width(icon_size as u16)
                        .height(icon_size as u16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(if is_active { Color::WHITE } else { theme.text_primary() }) })
                )
                .width(40)
                .height(40)
                .center_x(40)
                .center_y(40)
                .style(move |_| container::Style {
                    background: if is_active { Some(iced::Background::Color(theme.accent())) } else { None },
                    border: iced::Border {
                        color: if is_active { theme.accent() } else { theme.border() },
                        width: 1.0,
                        radius: 6.0.into(),
                    },
                    ..Default::default()
                }),
                text(label).size(10).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            ].spacing(8).align_x(Alignment::Center)
        )
        .on_press(match val {
            Some(v) => Message::SetScale(mon_name, v),
            None => Message::SetScaleAuto(mon_name),
        })
        .padding(0)
        .style(button::text)
        .into()
    })).spacing(20);

    let main_settings = card(theme, column![
        // Use As
        row![
            text("Use as").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            pick_list(
                vec!["Main display".to_string(), "Extended display".to_string()],
                Some(if mon.id == 0 { "Main display".to_string() } else { "Extended display".to_string() }),
                |_| Message::Tick(())
            ).width(200),
        ].align_y(Alignment::Center).padding([10, 10]),
        
        container(iced::widget::horizontal_rule(1)).padding([0, 0]),

        // Brightness
        row![
            svg(icons::icon_sun()).width(16).height(16).style(|_: &iced::Theme, _| svg::Style { color: Some(Color::from_rgb(0.5, 0.5, 0.5)) }),
            text("Brightness").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            slider(0..=100, app.brightness, Message::SetBrightness).width(200),
        ].spacing(12).align_y(Alignment::Center).padding([10, 10]),

        container(iced::widget::horizontal_rule(1)).padding([0, 0]),

        // Scaling Area
        column![
            text("Resolution").size(13).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            container(scaling_row).padding([10, 0]).width(Length::Fill).center_x(Length::Fill),
            
            row![
                text("Show all resolutions").size(12).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                toggler(false).on_toggle(|_| Message::Tick(()))
            ].padding([5, 0])
        ].spacing(10).padding([10, 10]).height(Length::Shrink),

        container(iced::widget::horizontal_rule(1)).padding([0, 0]),

        // Refresh Rate
        row![
            text("Refresh rate").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            pick_list(
                refresh_rates,
                Some(current_refresh),
                move |r| Message::SetResolution(name.clone(), current_res.clone(), r)
            ).width(200),
        ].align_y(Alignment::Center).padding([10, 10]),

        container(iced::widget::horizontal_rule(1)).padding([0, 0]),

        // Rotation
        row![
            text("Rotation").size(13).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            pick_list(
                vec!["Standard".to_string(), "90°".to_string(), "180°".to_string(), "270°".to_string()],
                Some(match mon.transform {
                    1 => "90°".to_string(),
                    2 => "180°".to_string(),
                    3 => "270°".to_string(),
                    _ => "Standard".to_string(),
                }),
                {
                    let mon_name = mon.name.clone();
                    move |rot| {
                        let transform = match rot.as_str() {
                            "90°" => 1,
                            "180°" => 2,
                            "270°" => 3,
                            _ => 0,
                        };
                        Message::SetTransform(mon_name.clone(), transform)
                    }
                }
            ).width(200),
        ].align_y(Alignment::Center).padding([10, 10]),
    ].spacing(0).height(Length::Shrink));

    let footer_buttons = row![
        button(text("Advanced...").size(12)).padding([6, 12]).style(button::secondary).on_press(Message::OpenDisplayAdvancedModal),
        button(text("Night Shift...").size(12)).padding([6, 12]).style(button::secondary).on_press(Message::OpenNightShiftModal),
    ].spacing(10);

    column![
        text(&mon.model).size(14).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
        main_settings,
        container(footer_buttons).width(Length::Fill).align_x(Alignment::End)
    ].spacing(12).height(Length::Shrink).into()
}

fn view_advanced_modal<'a>(_app: &'a App, theme: &'a MacOSTheme) -> Element<'a, Message> {
    let modal_content = column![
        text("Advanced Display Settings").size(20).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        
        text("These settings are currently managed by Hyprland's main configuration. For deep overrides, please edit ~/.config/hypr/hyprland.conf directly.")
            .size(13)
            .style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),

        container(
            row![
                button(text("Done").center())
                    .width(Length::Fixed(100.0))
                    .padding(10)
                    .on_press(Message::CloseDisplayAdvancedModal)
                    .style(move |_, _| {
                        let mut s = button::Style::default();
                        s.background = Some(iced::Background::Color(theme.accent()));
                        s.text_color = Color::WHITE;
                        s.border.radius = 8.0.into();
                        s
                    }),
            ]
        ).align_x(Alignment::End).width(Length::Fill)
    ].spacing(20);

    let modal_card = container(modal_content)
        .padding(24)
        .width(Length::Fixed(450.0))
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(theme.card_bg())),
            border: iced::Border {
                color: theme.border(),
                width: 1.0,
                radius: 12.0.into(),
            },
            shadow: iced::Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.3),
                offset: iced::Vector::new(0.0, 10.0),
                blur_radius: 30.0,
            },
            ..Default::default()
        });

    container(modal_card)
        .width(Length::Fill)
        .height(Length::Fill)
        .center_x(Length::Fill)
        .center_y(Length::Fill)
        .style(|_| container::Style {
            background: Some(iced::Background::Color(Color::from_rgba(0.0, 0.0, 0.0, 0.5))),
            ..Default::default()
        })
        .into()
}

fn view_night_shift_modal<'a>(app: &'a App, theme: &'a MacOSTheme) -> Element<'a, Message> {
    let modal_content = column![
        row![
            svg(icons::icon_moon()).width(24).height(24).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }),
            text("Night Shift").size(20).style(move |_| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
        ].spacing(10).align_y(Alignment::Center),

        card(theme, column![
            row![
                text("Turn On Until Tomorrow").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                toggler(app.night_shift_enabled).on_toggle(Message::ToggleNightShift)
            ].align_y(Alignment::Center).padding(10),
            
            container(iced::widget::horizontal_rule(1)).padding([0, 10]),
            
            column![
                row![
                    text("Color Temperature").size(14).width(Length::Fill).style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                ],
                row![
                    text("Less Warm").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    slider(2000..=6500, app.night_shift_temp, Message::SetNightShiftTemp).width(Length::Fill),
                    text("More Warm").size(12).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].spacing(15).align_y(Alignment::Center),
            ].spacing(10).padding(10),
        ].spacing(0)),

        text("Night Shift automatically shifts the colors of your display to the warmer end of the color spectrum after dark. This may help you get a better night's sleep.")
            .size(12)
            .style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),

        container(
            row![
                button(text("Done").center())
                    .width(Length::Fixed(100.0))
                    .padding(10)
                    .on_press(Message::CloseNightShiftModal)
                    .style(move |_, _| {
                        let mut s = button::Style::default();
                        s.background = Some(iced::Background::Color(theme.accent()));
                        s.text_color = Color::WHITE;
                        s.border.radius = 8.0.into();
                        s
                    }),
            ]
        ).align_x(Alignment::End).width(Length::Fill)
    ].spacing(20);

    let modal_card = container(modal_content)
        .padding(24)
        .width(Length::Fixed(450.0))
        .style(move |_| container::Style {
            background: Some(iced::Background::Color(theme.card_bg())),
            border: iced::Border {
                color: theme.border(),
                width: 1.0,
                radius: 12.0.into(),
            },
            shadow: iced::Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.3),
                offset: iced::Vector::new(0.0, 10.0),
                blur_radius: 30.0,
            },
            ..Default::default()
        });

    container(modal_card)
        .width(Length::Fill)
        .height(Length::Fill)
        .center_x(Length::Fill)
        .center_y(Length::Fill)
        .style(|_| container::Style {
            background: Some(iced::Background::Color(Color::from_rgba(0.0, 0.0, 0.0, 0.5))),
            ..Default::default()
        })
        .into()
}
