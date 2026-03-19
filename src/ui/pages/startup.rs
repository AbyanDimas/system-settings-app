use iced::{
    widget::{column, row, text, container, scrollable, svg},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let mut apps_list = column![].spacing(0);
    let len = app.startup_apps.len();

    if len == 0 {
        apps_list = apps_list.push(
            container(
                text("No startup items found.")
                    .size(14)
                    .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            )
            .padding(20)
            .center_x(Length::Fill)
        );
    } else {
        for (i, app_item) in app.startup_apps.iter().enumerate() {
            let is_enabled = !app.disabled_startup_apps.contains(&app_item.command);
            
            let item_row = row![
                container(
                    svg(icons::icon_rocket())
                        .width(16)
                        .height(16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_primary()) })
                )
                .width(32)
                .height(32)
                .center_x(32)
                .center_y(32)
                .style(move |_| container::Style {
                    background: Some(iced::Background::Color(if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                    border: iced::Border { radius: 8.0.into(), ..Default::default() },
                    ..Default::default()
                }),
                
                text(app_item.command.clone())
                    .size(14)
                    .width(Length::Fill)
                    .style(move |_| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    
                {
                    let cmd_for_msg = app_item.command.clone();
                    iced::widget::toggler(is_enabled).on_toggle(move |b| Message::ToggleStartupApp(cmd_for_msg.clone(), b))
                }
            ]
            .spacing(15)
            .align_y(Alignment::Center)
            .padding([12, 15]);

            apps_list = apps_list.push(item_row);

            if i < len - 1 {
                apps_list = apps_list.push(
                    container(iced::widget::horizontal_rule(1))
                        .padding(iced::Padding { top: 0.0, right: 15.0, bottom: 0.0, left: 60.0 })
                );
            }
        }
    }

    let content = column![
        text("Login Items").size(24).style(move |_| text::Style {
            color: Some(theme.text_primary()),
            ..Default::default()
        }),
        text("These applications open automatically when you log in.")
            .size(14)
            .style(move |_| text::Style {
                color: Some(theme.text_secondary()),
                ..Default::default()
            }),
            
        card(theme, apps_list),
    ]
    .spacing(20)
    .padding([40, 60])
    .width(Length::Fill); // Removed max_width to allow full responsiveness

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
