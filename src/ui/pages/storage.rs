use iced::{
    widget::{column, row, text, container, svg, scrollable, button, Space},
    Alignment, Element, Length, Color,
};
use crate::app::{App, Message, Route};
use crate::theme::card;
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    let content = if let Some(sys) = &app.sys_info {
        let details = &sys.storage_details;
        
        // Colors for the bar and legend
        let color_apps = Color::from_rgb8(255, 59, 48); // Red
        let color_docs = Color::from_rgb8(255, 149, 0); // Orange
        let color_music = Color::from_rgb8(255, 214, 10); // Yellow
        let color_downloads = Color::from_rgb8(52, 199, 89); // Green
        let color_pics = Color::from_rgb8(0, 199, 190); // Teal
        let color_videos = Color::from_rgb8(175, 82, 222); // Purple
        let color_system = Color::from_rgb8(142, 142, 147); // Grey
        let color_free = if theme.is_dark() { Color::from_rgb8(44, 44, 46) } else { Color::from_rgb8(229, 229, 234) };

        let total = details.total_gb;
        let used = details.used_gb;
        let free = (total - used).max(0.0);

        // Bar calculation
        let apps_w = details.apps_gb / total;
        let docs_w = details.documents_gb / total;
        let music_w = details.music_gb / total;
        let downloads_w = details.downloads_gb / total;
        let pics_w = details.pictures_gb / total;
        let videos_w = details.videos_gb / total;
        let system_w = (details.system_data_gb + details.other_gb) / total;
        let free_w = free / total;

        let storage_bar = container(
            row![
                container(Space::with_width(Length::FillPortion((apps_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_apps.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((docs_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_docs.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((music_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_music.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((downloads_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_downloads.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((pics_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_pics.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((videos_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_videos.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((system_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_system.into()), ..Default::default() }).height(Length::Fill),
                container(Space::with_width(Length::FillPortion((free_w * 1000.0) as u16))).style(move |_: &iced::Theme| container::Style { background: Some(color_free.into()), ..Default::default() }).height(Length::Fill),
            ]
            .width(Length::Fill)
            .height(24)
        )
        .width(Length::Fill)
        .clip(true)
        .style(move |_: &iced::Theme| container::Style {
            border: iced::Border { radius: 5.0.into(), ..Default::default() },
            ..Default::default()
        });

        let legend_item = |label: String, color: Color| {
            row![
                container(Space::with_width(8).height(8))
                    .style(move |_: &iced::Theme| container::Style { 
                        background: Some(color.into()), 
                        border: iced::Border { radius: 4.0.into(), ..Default::default() },
                        ..Default::default() 
                    }),
                text(label).size(11).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            ].spacing(6).align_y(Alignment::Center)
        };

        let legend = row![
            legend_item("Apps".to_string(), color_apps),
            legend_item("Documents".to_string(), color_docs),
            legend_item("Music".to_string(), color_music),
            legend_item("Downloads".to_string(), color_downloads),
            legend_item("Pics".to_string(), color_pics),
            legend_item("Videos".to_string(), color_videos),
            legend_item("System".to_string(), color_system),
        ].spacing(15).wrap();

        let recommendation_card = |title: String, desc: String, btn_label: String, icon: iced::widget::svg::Handle, msg: Message| {
            row![
                container(svg(icon).width(28).height(28).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }))
                    .padding(10),
                column![
                    text(title).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    text(desc).size(12).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].width(Length::Fill).spacing(4),
                button(text(btn_label).size(12))
                    .padding([6, 12])
                    .on_press(msg)
                    .style(move |_, status| {
                        let mut s = button::Style::default();
                        s.background = Some(iced::Background::Color(match status {
                            button::Status::Hovered => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) },
                            _ => if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                        }));
                        s.text_color = theme.text_primary();
                        s.border.radius = 6.0.into();
                        s
                    })
            ]
            .spacing(15)
            .align_y(Alignment::Center)
            .padding(15)
        };

        let category_item = |label: String, size: String, icon: iced::widget::svg::Handle, icon_color: Color| {
            let label_clone = label.clone();
            container(row![
                container(svg(icon).width(16).height(16).style(move |_: &iced::Theme, _| svg::Style { color: Some(Color::WHITE) }))
                    .width(28).height(28).center_x(28).center_y(28)
                    .style(move |_: &iced::Theme| container::Style { 
                        background: Some(icon_color.into()), 
                        border: iced::Border { radius: 6.0.into(), ..Default::default() },
                        ..Default::default() 
                    }),
                text(label).size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                text(size).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                button(
                    svg(icons::icon_info()).width(16).height(16).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
                )
                .on_press(Message::OpenStorageDetails(label_clone))
                .style(button::text)
            ].spacing(12).align_y(Alignment::Center).padding([10, 15]))
        };

        column![
            row![
                button(
                    row![
                        svg(icons::icon_chevron_right()).width(16).height(16)
                            .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                        text("General").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                    ].spacing(5).align_y(Alignment::Center)
                )
                .padding([5, 0])
                .on_press(Message::Navigate(Route::System))
                .style(button::text),
                
                Space::with_width(Length::Fill),
                
                button(
                    svg(icons::icon_refresh()).width(16).height(16)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
                )
                .on_press(Message::RefreshSecurity) // Using this as a generic refresh for now
                .style(button::text)
            ].align_y(Alignment::Center),

            text("Storage").size(24).style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),

            card(theme, column![
                row![
                    text(&details.disk_name).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                    Space::with_width(Length::Fill),
                    text(format!("{:.2} GB of {:.2} GB used", used, total)).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                ].padding([15, 20]),
                
                container(column![
                    storage_bar,
                    legend,
                ].spacing(15)).padding([10, 20]),
            ]),

            text("Recommendations").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
            
            card(theme, column![
                recommendation_card("Store in Omarchy Cloud".to_string(), "Store all files, photos, and messages in cloud and save space.".to_string(), "Store in Cloud...".to_string(), icons::icon_globe(), Message::StoreInCloud),
                iced::widget::horizontal_rule(1),
                recommendation_card("Optimize Storage".to_string(), "Save space by automatically removing old log files and temporary data.".to_string(), "Optimize...".to_string(), icons::icon_activity(), Message::OptimizeStorage),
                iced::widget::horizontal_rule(1),
                recommendation_card("Empty Trash".to_string(), "Free up space by permanently erasing items in the Trash.".to_string(), "Empty...".to_string(), icons::icon_trash(), Message::EmptyTrash),
            ]),

            card(theme, column![
                category_item("Applications".to_string(), details.apps.clone(), icons::icon_apps(), Color::from_rgb8(0, 122, 255)),
                iced::widget::horizontal_rule(1),
                category_item("Documents".to_string(), details.documents.clone(), icons::icon_book(), Color::from_rgb8(142, 142, 147)),
                iced::widget::horizontal_rule(1),
                category_item("Music".to_string(), details.music.clone(), icons::icon_music(), Color::from_rgb8(255, 59, 48)),
                iced::widget::horizontal_rule(1),
                category_item("Downloads".to_string(), details.downloads.clone(), icons::icon_download(), Color::from_rgb8(52, 199, 89)),
                iced::widget::horizontal_rule(1),
                category_item("Pictures".to_string(), details.pictures.clone(), icons::icon_image(), Color::from_rgb8(0, 199, 190)),
                iced::widget::horizontal_rule(1),
                category_item("Videos".to_string(), details.videos.clone(), icons::icon_video(), Color::from_rgb8(175, 82, 222)),
                iced::widget::horizontal_rule(1),
                category_item("Trash".to_string(), details.trash.clone(), icons::icon_trash(), Color::from_rgb8(142, 142, 147)),
            ]),
            
            Space::with_height(20),
        ]
        .spacing(20)
        .padding(iced::Padding { top: 20.0, right: 60.0, bottom: 40.0, left: 60.0 })
        .width(Length::Fill)
    } else {
        column![text("Loading...")]
    };

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
