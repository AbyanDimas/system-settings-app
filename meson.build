project('system-settings', 'c',
  version: '0.1.0',
  meson_version: '>= 0.62.0',
  default_options: [ 'warning_level=2', 'c_std=c11' ],
)

gtk_dep = dependency('gtk4')
adw_dep = dependency('libadwaita-1')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
sourceview_dep = dependency('gtksourceview-5')

sources = [
  'src/main.c',
  'src/app.c',
  'src/window.c',
  'src/utils/subprocess.c',
  'src/utils/config-parser.c',
  'src/utils/tool-detector.c',

  # Connectivity
  'src/pages/page-network.c',
  'src/pages/page-hotspot.c',
  'src/pages/page-bluetooth.c',
  'src/pages/page-vpn.c',
  'src/pages/page-hosts.c',
  'src/pages/page-netmonitor.c',
  'src/pages/page-monitor.c',

  # Notifications & Focus
  'src/pages/page-notifications.c',
  'src/pages/page-focus.c',
  'src/pages/page-screentime.c',

  # General & System
  'src/pages/page-general.c',
  'src/pages/page-sysinfo.c',
  'src/pages/page-displays.c',
  'src/pages/page-sound.c',
  'src/pages/page-battery.c',
  'src/pages/page-privacy.c',
  'src/pages/page-update.c',
  'src/pages/page-snapshots.c',
  'src/pages/page-storage.c',
  'src/pages/page-hardware.c',
  'src/pages/page-services.c',
  'src/pages/page-logs.c',

  # Input & Desktop
  'src/pages/page-appearance.c',
  'src/pages/page-waybar.c',
  'src/pages/page-wallpaper.c',
  'src/pages/page-touchpad.c',
  'src/pages/page-startup.c',
  'src/pages/page-looknfeel.c',
  'src/pages/page-shaders.c',
  'src/pages/page-keybinds.c',
  'src/pages/page-input.c',
  'src/pages/page-hypridle.c',
  'src/pages/page-hyprlock.c',

  # Apps & Customization
  'src/pages/page-terminal.c',
  'src/pages/page-walker.c',
  'src/pages/page-shell.c',
  'src/pages/page-theme.c',
  'src/pages/page-fonts.c',
  'src/pages/page-xcompose.c',
  'src/pages/page-starship.c',
  'src/pages/page-filetools.c',

  # Security
  'src/pages/page-fingerprint.c',
  'src/pages/page-security.c',
  'src/pages/page-firewall.c',
  'src/pages/page-intrusion.c',
  'src/pages/page-ssh.c',
  'src/pages/page-encryption.c',
  'src/pages/page-gpg.c',
  'src/pages/page-privacy-dashboard.c',
  'src/pages/page-app-permissions.c',

  # Advanced & Dev
  'src/pages/page-docker.c',
  'src/pages/page-ollama.c',
  'src/pages/page-gamemode.c',
  'src/pages/page-mise.c',
  'src/pages/page-github.c',
  'src/pages/page-ai.c',
  'src/pages/page-windowsvm.c',

  # Newly Added Sections
  'src/pages/page-users.c',
  'src/pages/page-locale.c',
  'src/pages/page-defaultapps.c',
  'src/pages/page-printers.c',
  'src/pages/page-pacman.c',
  'src/pages/page-accessibility.c',
  'src/pages/page-boot.c',
]

executable('system-settings',
  sources,
  dependencies: [gtk_dep, adw_dep, glib_dep, gio_dep, sourceview_dep],
  install: true
)

install_data('data/system-settings.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications')
)
