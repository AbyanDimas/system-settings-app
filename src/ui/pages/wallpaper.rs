use iced::{
    widget::{column, row, text, container, scrollable, svg, button, image, Space},
    Alignment, Element, Length, Color,
};
use std::fs;
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // Get current wallpaper path - resolve symlink to get the real file path for iced::image
    let wallpaper_path_str = format!("{}/.config/omarchy/current/background", std::env::var("HOME").unwrap_or_default());
    let wallpaper_path = fs::canonicalize(&wallpaper_path_str)
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or(wallpaper_path_str);

    // Header
    let header = column![
        text("Wallpaper").size(28).style(move |_: &iced::Theme| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        text("Select a background image for your desktop.")
            .size(14)
            .style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),
    ]
    .spacing(8);

    // Current Wallpaper Preview (macOS Style Banner)
    let current_preview = container(
        image(wallpaper_path.clone())
            .content_fit(iced::ContentFit::Cover)
            .width(Length::Fill)
            .height(Length::Fill)
    )
    .width(Length::Fill)
    .height(Length::Fixed(240.0))
    .clip(true)
    .style(move |_: &iced::Theme| container::Style {
        border: iced::Border {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.1),
            width: 1.0,
            radius: 12.0.into(),
        },
        shadow: iced::Shadow {
            color: Color::from_rgba(0.0, 0.0, 0.0, 0.1),
            offset: iced::Vector::new(0.0, 4.0),
            blur_radius: 12.0,
        },
        ..Default::default()
    });

    // Quick Settings Card
    let quick_settings = card(theme, column![
        row![
            svg(icons::icon_image()).width(16).height(16).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }),
            column![
                text("Current Wallpaper").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(wallpaper_path.split('/').last().unwrap_or("Unknown").to_string()).size(11).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
            ].width(Length::Fill),
            button(text("Change Wallpaper...").size(12))
                .padding([6, 12])
                .on_press(Message::Tick(())) // Placeholder for file picker
                .style(move |_, status| {
                    let mut s = button::Style::default();
                    s.background = Some(iced::Background::Color(theme.card_bg()));
                    s.border = iced::Border { color: theme.border(), width: 1.0, radius: 6.0.into() };
                    s.text_color = theme.text_primary();
                    if status == button::Status::Hovered {
                        s.background = Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }));
                    }
                    s
                }),
        ].align_y(Alignment::Center).spacing(12),
    ]);

    // Wallpaper Library Grid
    let home = std::env::var("HOME").unwrap_or_default();
    let library_items = vec![
        (format!("{}/.config/omarchy/backgrounds/catppuccin/dark-gradient-6bly12umg2d4psr2.jpg", home), "Catppuccin Gradient"),
        (format!("{}/.config/omarchy/backgrounds/catppuccin-dark/mountain.png", home), "Omarchy Mountain"),
        (format!("{}/.config/omarchy/backgrounds/catppuccin-latte/Cutefish OS.heic", home), "Cutefish Latte"),
    ];

    let mut library_grid = row![].spacing(15).width(Length::Fill);
    for (path, name) in library_items {
        let is_selected = wallpaper_path == path;
        
        let item = button(
            column![
                container(
                    image(path.clone())
                        .content_fit(iced::ContentFit::Cover)
                        .width(Length::Fill)
                        .height(Length::Fill)
                )
                .width(Length::Fill)
                .height(Length::Fixed(100.0))
                .clip(true)
                .style(move |_: &iced::Theme| container::Style {
                    border: iced::Border {
                        radius: 8.0.into(),
                        width: if is_selected { 2.0 } else { 0.0 },
                        color: theme.accent(),
                    },
                    ..Default::default()
                }),
                text(name).size(11).style(move |_: &iced::Theme| text::Style {
                    color: Some(if is_selected { theme.accent() } else { theme.text_primary() }),
                    ..Default::default()
                })
            ].spacing(6).align_x(Alignment::Center)
        )
        .on_press(Message::SetWallpaper(path))
        .width(Length::FillPortion(1))
        .style(move |_, _| button::Style::default());

        library_grid = library_grid.push(item);
    }

    let library_section = column![
        text("Library").size(16).style(move |_: &iced::Theme| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        library_grid,
    ].spacing(12);

    let content = column![
        header,
        current_preview,
        quick_settings,
        library_section,
        Space::with_height(20),
    ]
    .spacing(30)
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
