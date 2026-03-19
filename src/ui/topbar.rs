use iced::{
    widget::{button, container, row, text, text_input, svg},
    Alignment, Element, Length,
};

use crate::app::{App, Message, Route};

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;
    let title = app.title();

    let back_icon_svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m15 18-6-6 6-6"/></svg>"#;
    let back_icon = svg(svg::Handle::from_memory(back_icon_svg.as_bytes()))
        .width(18)
        .height(18)
        .style(move |_: &iced::Theme, _| svg::Style {
            color: Some(theme.text_primary()),
        });

    let back_button = if app.route != Route::Home {
        button(back_icon)
            .on_press(Message::NavigateBack)
            .padding([8, 12])
            .style(move |_, status| {
                let mut style = button::Style::default();
                style.background = match status {
                    button::Status::Hovered => Some(iced::Background::Color(
                        if theme.is_dark() {
                            iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05)
                        } else {
                            iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05)
                        }
                    )),
                    _ => None,
                };
                style.border.radius = 8.0.into();
                style
            })
    } else {
        button(text("").size(20)).style(button::text).padding([8, 12])
    };

    let title_text = text(title)
        .size(16)
        .width(Length::Fill)
        .center();

    let search_input = text_input("Search", &app.search_query)
        .on_input(Message::SearchChanged)
        .width(Length::Fixed(200.0))
        .padding(8);

    container(
        row![back_button, title_text, search_input]
            .align_y(Alignment::Center)
            .padding([12, 20])
            .spacing(10),
    )
    .width(Length::Fill)
    .style(move |_| container::Style {
        background: Some(iced::Background::Color(theme.sidebar_bg())),
        border: iced::Border {
            color: theme.border(),
            width: 0.0,
            ..Default::default()
        },
        ..Default::default()
    })
    .into()
}
