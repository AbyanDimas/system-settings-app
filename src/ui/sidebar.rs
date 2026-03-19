use iced::{
    widget::{button, column, container, row, text, svg, text_input, scrollable, stack},
    Alignment, Element, Length, Color,
};

use crate::app::{App, Message, Route};
use crate::ui::icons;

struct SidebarItem {
    label: &'static str,
    route: Route,
    icon: iced::widget::svg::Handle,
    color: Color,
    keywords: &'static str,
}

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    // Search bar with macOS-like appearance
    let search_input = text_input("Search", &app.search_query)
        .on_input(Message::SearchChanged)
        .size(13)
        .padding(iced::Padding {
            top: 6.0,
            right: 8.0,
            bottom: 6.0,
            left: 28.0,
        }); // Left padding for search icon

    let search_bar = container(
        stack![
            search_input,
            container(
                svg(icons::icon_search())
                    .width(14)
                    .height(14)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) })
            )
            .padding([8, 8])
            .align_x(Alignment::Start)
            .align_y(Alignment::Center)
        ]
    ).padding(iced::Padding { top: 20.0, right: 15.0, bottom: 15.0, left: 15.0 });

    // macOS Style Sidebar Items with colored icon backgrounds
    let categorized_items = vec![
        ("Connectivity", vec![
            SidebarItem { label: "Network", route: Route::Network, icon: icons::icon_network(), color: Color::from_rgb8(0, 122, 255), keywords: "wifi internet network connection" },
            SidebarItem { label: "Hotspot", route: Route::Hotspot, icon: icons::icon_hotspot(), color: Color::from_rgb8(52, 199, 89), keywords: "hotspot tethering sharing wifi" },
            SidebarItem { label: "Bluetooth", route: Route::Bluetooth, icon: icons::icon_bluetooth(), color: Color::from_rgb8(0, 122, 255), keywords: "bluetooth wireless devices pairing" },
        ]),
        ("Notifications", vec![
            SidebarItem { label: "Notifications", route: Route::Notifications, icon: icons::icon_bell(), color: Color::from_rgb8(255, 59, 48), keywords: "notifications alerts banners" },
            SidebarItem { label: "Focus", route: Route::Focus, icon: icons::icon_moon(), color: Color::from_rgb8(88, 86, 214), keywords: "focus do not disturb dnd quiet" },
            SidebarItem { label: "Screen Time", route: Route::ScreenTime, icon: icons::icon_hourglass(), color: Color::from_rgb8(175, 82, 222), keywords: "screen time usage tracking limits" },
        ]),
        ("General", vec![
            SidebarItem { label: "General", route: Route::System, icon: icons::icon_system(), color: Color::from_rgb8(142, 142, 147), keywords: "general system about info update storage airdrop accessibility startup login gaming game language region time sharing backup" },
            SidebarItem { label: "Displays", route: Route::Display, icon: icons::icon_display(), color: Color::from_rgb8(0, 122, 255), keywords: "display monitor resolution brightness scaling screen" },
            SidebarItem { label: "Sound", route: Route::Audio, icon: icons::icon_audio(), color: Color::from_rgb8(255, 59, 48), keywords: "sound audio volume speaker microphone" },
            SidebarItem { label: "Battery & Power", route: Route::Power, icon: icons::icon_power(), color: Color::from_rgb8(52, 199, 89), keywords: "battery power energy performance charging sleep display" },
            SidebarItem { label: "Privacy & Security", route: Route::Security, icon: icons::icon_shield(), color: Color::from_rgb8(142, 142, 147), keywords: "privacy security firewall ssh password users login audit" },
        ]),
        ("Customization", vec![
            SidebarItem { label: "Appearance", route: Route::Appearance, icon: icons::icon_appearance(), color: Color::from_rgb8(28, 28, 30), keywords: "appearance theme dark light mode" },
            SidebarItem { label: "Waybar", route: Route::Waybar, icon: icons::icon_waybar(), color: Color::from_rgb8(52, 199, 89), keywords: "waybar taskbar panel statusbar" },
            SidebarItem { label: "Wallpaper", route: Route::Wallpaper, icon: icons::icon_image(), color: Color::from_rgb8(90, 200, 250), keywords: "wallpaper background image desktop" },
            SidebarItem { label: "Touchpad & Mouse", route: Route::TouchpadMouse, icon: icons::icon_touchpad(), color: Color::from_rgb8(142, 142, 147), keywords: "touchpad mouse gestures scrolling" },
            SidebarItem { label: "Startup Apps", route: Route::Startup, icon: icons::icon_rocket(), color: Color::from_rgb8(255, 149, 0), keywords: "startup apps login autostart" },
            SidebarItem { label: "Keybinds", route: Route::Keybinds, icon: icons::icon_keybinds(), color: Color::from_rgb8(142, 142, 147), keywords: "keybinds shortcuts keyboard" },
        ]),
    ];

    let query = app.search_query.to_lowercase();
    
    let mut nav_list = column![].spacing(15).padding(iced::Padding { top: 0.0, right: 10.0, bottom: 20.0, left: 10.0 });
    let mut found_any = false;

    for (category, items) in categorized_items {
        let filtered_items: Vec<_> = items.into_iter()
            .filter(|item| {
                query.is_empty() || 
                item.label.to_lowercase().contains(&query) || 
                item.keywords.contains(&query)
            })
            .collect();

        if !filtered_items.is_empty() {
            found_any = true;
            let mut category_col = column![];
            
            // Only show category headers if not searching, or optionally if you want to keep them
            if query.is_empty() {
                 category_col = category_col.push(
                     container(text(category).size(11).style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() }))
                         .padding(iced::Padding { top: 5.0, right: 10.0, bottom: 5.0, left: 10.0 })
                 );
            }

            let mut items_col = column![].spacing(2);

            for item in filtered_items {
                let is_active = app.route == item.route;
                
                let icon_box = container(
                    svg(item.icon)
                        .width(14)
                        .height(14)
                        .style(move |_: &iced::Theme, _| svg::Style {
                            color: Some(Color::WHITE),
                        })
                )
                .width(24)
                .height(24)
                .center_x(24)
                .center_y(24)
                .style(move |_| container::Style {
                    background: Some(iced::Background::Color(item.color)),
                    border: iced::Border { radius: 6.0.into(), ..Default::default() },
                    ..Default::default()
                });

                let content = row![
                    icon_box,
                    text(item.label).size(13).style(move |_| text::Style {
                        color: Some(if is_active { Color::WHITE } else { theme.text_primary() }),
                        ..Default::default()
                    }),
                ]
                .spacing(10)
                .align_y(Alignment::Center);

                let btn = button(content)
                    .width(Length::Fill)
                    .padding([6, 10])
                    .on_press(Message::Navigate(item.route.clone()))
                    .style(move |_, status| {
                        let mut style = button::Style::default();
                        if is_active {
                            style.background = Some(iced::Background::Color(theme.accent()));
                            style.text_color = Color::WHITE;
                        } else {
                            style.background = match status {
                                button::Status::Hovered => Some(iced::Background::Color(
                                    if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } 
                                    else { Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                                )),
                                _ => None,
                            };
                        }
                        style.border.radius = 6.0.into();
                        style
                    });

                items_col = items_col.push(btn);
            }
            category_col = category_col.push(items_col);
            nav_list = nav_list.push(category_col);
        }
    }

    if !found_any && !query.is_empty() {
        nav_list = nav_list.push(
            container(
                text("No results found")
                    .size(12)
                    .style(move |_| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
            )
            .width(Length::Fill)
            .center_x(Length::Fill)
            .padding(20)
        );
    }

    container(
        column![
            search_bar,
            scrollable(nav_list).direction(
                iced::widget::scrollable::Direction::Vertical(
                    iced::widget::scrollable::Scrollbar::new().width(0).scroller_width(0).margin(0)
                )
            )
        ]
    )
    .width(Length::Fixed(240.0))
    .height(Length::Fill)
    .style(move |_| container::Style {
        background: Some(iced::Background::Color(theme.sidebar_bg())),
        border: iced::Border {
            color: if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) },
            width: 1.0,
            ..Default::default()
        },
        ..Default::default()
    })
    .into()
}
