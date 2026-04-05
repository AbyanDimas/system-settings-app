# Omarchy Settings

A system settings application for Linux and Hyprland desktops, built with C, GTK4, and Libadwaita.

## 🚀 Features

Omarchy Settings provides a central hub for managing your Linux desktop, with sections including:

- **Connectivity**: Network, Bluetooth, VPN, and Network Monitor.
- **System Info**: Detailed hardware and software information, battery, storage, and system updates.
- **Appearance**: Dark/Light mode, theme selection, wallpaper management, and desktop styling.
- **Input & Desktop**: Keyboard, touchpad, and Hyprland-specific settings like Hypridle and Hyprlock.
- **Security**: Fingerprint, firewall, SSH, and GPG management.
- **Advanced**: Docker, Ollama (AI), GitHub integration, and Windows VM control.

## 🛠️ Build Dependencies

Ensure you have the following installed:

- `meson` >= 0.62.0
- `ninja`
- `gcc` or `clang`
- `gtk4`
- `libadwaita-1`
- `gtksourceview-5`
- `glib-2.0`
- `gio-2.0`

## 🏗️ Building and Running

### 1. Configure the build directory
```bash
meson setup build
```

### 2. Compile the project
```bash
meson compile -C build
```

### 3. Run the application
```bash
./build/system-settings
```

## 📦 Installation

To install the application system-wide:
```bash
sudo meson install -C build
```

## 🧪 Testing

To run the project tests:
```bash
./run-tests.sh
```
OR
```bash
meson test -v -C build
```
