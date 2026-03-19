use iced::{widget::{column, row, text, toggler, container, svg, scrollable, button, text_input}, Element, Alignment, Length, Color};
use crate::app::{App, Message};
use crate::theme::{card, MacOSTheme};
use crate::ui::icons;

pub fn view(app: &App) -> Element<'_, Message> {
    let theme = &app.theme;

    if app.show_bluetooth_rename_modal {
        return view_rename_modal(app, theme);
    }

    let mut devices_col = column![].spacing(10);
    let mut has_devices = false;

    if app.bluetooth_on {
        for dev in &app.bluetooth_devices {
            let path = dev.object_path.clone();
            let is_connecting = app.connecting_device_path.as_ref() == Some(&path);
            
            let dev_icon = match dev.icon.as_str() {
                "phone" | "smartphone" => icons::icon_smartphone(),
                "computer" | "laptop" => icons::icon_laptop(),
                "audio-card" | "audio-headset" | "headset" | "headphones" => icons::icon_headphones(),
                _ => icons::icon_bluetooth(),
            };

            let action_button: Element<'_, Message> = if dev.connected {
                button(
                    row![
                        svg(icons::icon_check()).width(14).height(14).style(move |_: &iced::Theme, _| svg::Style { color: Some(iced::Color::WHITE) }),
                        text("Connected").size(11)
                    ].spacing(5).align_y(Alignment::Center)
                )
                .padding([6, 12])
                .on_press(Message::DisconnectBluetooth(path))
                .style(move |_, _| {
                    let mut s = button::Style::default();
                    s.background = Some(iced::Background::Color(theme.accent()));
                    s.text_color = iced::Color::WHITE;
                    s.border.radius = 6.0.into();
                    s
                })
                .into()
            } else if is_connecting {
                button(
                    row![
                        text("Connecting...").size(11)
                    ].spacing(5).align_y(Alignment::Center)
                )
                .padding([6, 12])
                .style(move |_, _| {
                    let mut s = button::Style::default();
                    s.background = Some(iced::Background::Color(
                        if theme.is_dark() { Color::from_rgba(1.0, 1.0, 1.0, 0.1) } else { Color::from_rgba(0.0, 0.0, 0.0, 0.1) }
                    ));
                    s.text_color = theme.text_secondary();
                    s.border.radius = 6.0.into();
                    s
                })
                .into()
            } else {
                button(text("Connect").size(11))
                    .on_press(Message::ConnectBluetooth(path))
                    .padding([6, 12])
                    .style(move |_, status| {
                        let mut s = button::Style::default();
                        s.background = match status {
                            button::Status::Hovered => Some(iced::Background::Color(
                                if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.1) }
                                else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) }
                            )),
                            _ => Some(iced::Background::Color(
                                if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) }
                                else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.02) }
                            )),
                        };
                        s.text_color = theme.text_primary();
                        s.border.radius = 6.0.into();
                        s.border.color = theme.border();
                        s.border.width = 1.0;
                        s
                    })
                    .into()
            };

            let mut status_row = row![].spacing(10).align_y(Alignment::Center);
            
            if let Some(pct) = dev.battery_percentage {
                status_row = status_row.push(
                    row![
                        svg(if pct < 20 { icons::icon_battery_low() } else { icons::icon_battery_full() })
                            .width(12).height(12).style(move |_: &iced::Theme, _| svg::Style { color: Some(if pct < 20 { Color::from_rgb(0.8, 0.2, 0.2) } else { Color::from_rgb(0.2, 0.8, 0.2) }) }),
                        text(format!("{}%", pct)).size(11).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                    ].spacing(4).align_y(Alignment::Center)
                );
            }

            if let Some(rssi) = dev.rssi {
                status_row = status_row.push(
                    row![
                        svg(icons::icon_signal()).width(12).height(12).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.text_secondary()) }),
                        text(format!("{}dBm", rssi)).size(11).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
                    ].spacing(4).align_y(Alignment::Center)
                );
            }

            devices_col = devices_col.push(card(theme, row![
                container(
                    svg(dev_icon)
                        .width(18)
                        .height(18)
                        .style(move |_: &iced::Theme, _| svg::Style { color: Some(if dev.connected { theme.accent() } else { theme.text_secondary() }) })
                )
                .padding(10)
                .style(move |_: &iced::Theme| container::Style {
                    background: Some(iced::Background::Color(if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                    border: iced::Border {
                        radius: 10.0.into(),
                        ..Default::default()
                    },
                    ..Default::default()
                }),
                column![
                    text(&dev.name).size(14).style(move |_: &iced::Theme| text::Style {
                        color: Some(theme.text_primary()),
                        ..Default::default()
                    }),
                    row![
                        text(if dev.connected { "Connected" } else if dev.paired { "Paired" } else { "Available" })
                            .size(12)
                            .style(move |_: &iced::Theme| text::Style {
                                color: Some(theme.text_secondary()),
                                ..Default::default()
                            }),
                        status_row,
                    ].spacing(15).align_y(Alignment::Center),
                ].width(Length::Fill).spacing(2),
                action_button
            ].spacing(15).align_y(Alignment::Center)));
            has_devices = true;
        }
    }

    // Animation for scanning
    let _scan_rotation = (app.last_tick.elapsed().as_millis() % 2000) as f32 / 2000.0 * 360.0;

    let mut content = column![
        row![
            text("Bluetooth").size(24).style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }).width(Length::Fill),
            if app.bluetooth_on {
                button(
                    row![
                        iced::widget::container(
                            svg(icons::icon_refresh()).width(14).height(14)
                                .style(move |_: &iced::Theme, _| svg::Style { color: Some(if app.is_bluetooth_scanning { theme.accent() } else { theme.text_secondary() }) })
                        )
                        .center_x(14)
                        .center_y(14),
                        text(if app.is_bluetooth_scanning { "Scanning..." } else { "Scan" }).size(12)
                    ].spacing(8).align_y(Alignment::Center)
                )
                .on_press(if app.is_bluetooth_scanning { Message::StopBluetoothDiscovery } else { Message::StartBluetoothDiscovery })
                .style(button::text)
            } else {
                button(text("")).style(button::text)
            }
        ].align_y(Alignment::Center),
        
        card(theme, row![
            container(
                svg(if app.bluetooth_on { icons::icon_bluetooth() } else { icons::icon_bluetooth_off() })
                    .width(24)
                    .height(24)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(if app.bluetooth_on { theme.accent() } else { theme.text_secondary() }) })
            )
            .padding(10)
            .style(move |_: &iced::Theme| container::Style {
                background: Some(iced::Background::Color(if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                border: iced::Border {
                    radius: 10.0.into(),
                    ..Default::default()
                },
                ..Default::default()
            }),
            column![
                text("Bluetooth").size(16).style(move |_: &iced::Theme| text::Style {
                    color: Some(theme.text_primary()),
                    ..Default::default()
                }),
                row![
                    text(if app.bluetooth_on { format!("Visible as \"{}\"", app.bluetooth_adapter_name) } else { "Off".to_string() })
                        .size(12)
                        .style(move |_: &iced::Theme| text::Style {
                            color: Some(theme.text_secondary()),
                            ..Default::default()
                        }),
                    if app.bluetooth_on {
                        button(svg(icons::icon_edit()).width(12).height(12).style(move |_: &iced::Theme, _| svg::Style { color: Some(theme.accent()) }))
                            .on_press(Message::OpenBluetoothRenameModal)
                            .style(button::text)
                            .padding(0)
                    } else {
                        button(text("")).style(button::text)
                    }
                ].spacing(8).align_y(Alignment::Center),
            ].width(Length::Fill).spacing(2),
            toggler(app.bluetooth_on).on_toggle(Message::SetBluetooth),
        ].spacing(15).align_y(Alignment::Center)),

        card(theme, row![
            container(
                svg(icons::icon_rocket())
                    .width(24)
                    .height(24)
                    .style(move |_: &iced::Theme, _| svg::Style { color: Some(if app.bluetooth_auto_on { theme.accent() } else { theme.text_secondary() }) })
            )
            .padding(10)
            .style(move |_: &iced::Theme| container::Style {
                background: Some(iced::Background::Color(if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.05) } else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.05) })),
                border: iced::Border {
                    radius: 10.0.into(),
                    ..Default::default()
                },
                ..Default::default()
            }),
            column![
                text("Start Bluetooth on boot").size(16).style(move |_: &iced::Theme| text::Style {
                    color: Some(theme.text_primary()),
                    ..Default::default()
                }),
                text("Enable or disable Bluetooth automatically when you log in.")
                    .size(12)
                    .style(move |_: &iced::Theme| text::Style {
                        color: Some(theme.text_secondary()),
                        ..Default::default()
                    }),
            ].width(Length::Fill).spacing(2),
            toggler(app.bluetooth_auto_on).on_toggle(Message::SetBluetoothAutoOn),
        ].spacing(15).align_y(Alignment::Center)),
    ].spacing(20);

    if app.bluetooth_on {
        if let Some(adapter) = &app.bluetooth_adapter {
            content = content.push(column![
                text("Adapter Details").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                card(theme, column![
                    row![
                        text("Address").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        text(&adapter.address).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].padding([12, 15]),
                    iced::widget::horizontal_rule(1)
,
                    row![
                        text("Pairable").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        text(if adapter.pairable { "On" } else { "Off" }).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].padding([12, 15]),
                    iced::widget::horizontal_rule(1)
,
                    row![
                        text("Discoverable").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_primary()), ..Default::default() }),
                        text(if adapter.discoverable { "On" } else { "Off" }).size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    ].padding([12, 15]),
                    if !adapter.modalias.is_empty() {
                        let chip = adapter.modalias.split(':').last().unwrap_or(&adapter.modalias);
                        let tp = theme.text_primary();
                        let ts = theme.text_secondary();
                        iced::Element::from(column![
                            iced::widget::horizontal_rule(1),
                            row![
                                text("Hardware ID").size(14).width(Length::Fill).style(move |_: &iced::Theme| text::Style { color: Some(tp), ..Default::default() }),
                                text(chip.to_string()).size(14).style(move |_: &iced::Theme| text::Style { color: Some(ts), ..Default::default() }),
                            ].padding([12, 15]),
                        ])
                    } else {
                        iced::widget::Space::with_height(0).into()
                    }
                ])
            ].spacing(10));
        }

        if has_devices {
            content = content.push(column![
                text("Devices").size(14).style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                devices_col,
            ].spacing(10));
        } else {
            content = content.push(container(
                column![
                    text("Searching for devices...")
                        .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() }),
                    if app.is_bluetooth_scanning {
                         iced::Element::from(text("Please ensure your device is in pairing mode."))
                    } else {
                         iced::Element::from(button("Start Scan").on_press(Message::StartBluetoothDiscovery).style(button::text))
                    }
                ].spacing(10).align_x(Alignment::Center)
            )
            .width(Length::Fill)
            .center_x(Length::Fill)
            .padding(40));
        }
    } else {
        content = content.push(container(
            text("Bluetooth is turned off.")
                .style(move |_: &iced::Theme| text::Style { color: Some(theme.text_secondary()), ..Default::default() })
        )
        .width(Length::Fill)
        .center_x(Length::Fill)
        .padding(40));
    }

    let content = content.padding(30);

    scrollable(container(content).width(Length::Fill))
        .direction(iced::widget::scrollable::Direction::Vertical(
            iced::widget::scrollable::Scrollbar::new()
                .width(6)
                .scroller_width(6)
                .margin(2)
        ))
        .into()
}

fn view_rename_modal<'a>(app: &'a App, theme: &'a MacOSTheme) -> Element<'a, Message> {
    let modal_content = column![
        text("Rename Bluetooth Adapter")
            .size(16)
            .style(move |_: &iced::Theme| text::Style {
                color: Some(theme.text_primary()),
                ..Default::default()
            }),
        
        text_input("Device Name", &app.new_bluetooth_name)
            .on_input(Message::NewBluetoothNameChanged)
            .padding(10)
            .on_submit(Message::SubmitBluetoothRename),

        row![
            button(text("Cancel").center())
                .width(Length::Fill)
                .padding(10)
                .on_press(Message::CloseBluetoothRenameModal)
                .style(move |_, _| {
                    let mut s = button::Style::default();
                    s.background = Some(iced::Background::Color(
                        if theme.is_dark() { iced::Color::from_rgba(1.0, 1.0, 1.0, 0.1) } 
                        else { iced::Color::from_rgba(0.0, 0.0, 0.0, 0.1) }
                    ));
                    s.text_color = theme.text_primary();
                    s.border.radius = 8.0.into();
                    s
                }),
            button(text("Rename").center())
                .width(Length::Fill)
                .padding(10)
                .on_press(Message::SubmitBluetoothRename)
                .style(move |_, _| {
                    let mut s = button::Style::default();
                    s.background = Some(iced::Background::Color(theme.accent()));
                    s.text_color = iced::Color::WHITE;
                    s.border.radius = 8.0.into();
                    s
                }),
        ].spacing(10)
    ].spacing(20);

    let modal_card = container(modal_content)
        .padding(24)
        .width(Length::Fixed(400.0))
        .style(move |_: &iced::Theme| container::Style {
            background: Some(iced::Background::Color(theme.card_bg())),
            border: iced::Border {
                color: theme.border(),
                width: 1.0,
                radius: 12.0.into(),
            },
            shadow: iced::Shadow {
                color: iced::Color::from_rgba(0.0, 0.0, 0.0, 0.3),
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
            background: Some(iced::Background::Color(iced::Color::from_rgba(0.0, 0.0, 0.0, 0.5))),
            ..Default::default()
        })
        .into()
}

