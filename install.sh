#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
# install.sh — Build & install Omarchy System Settings as a
#              proper Linux application.
#
# Usage:
#   chmod +x install.sh
#   ./install.sh          # build release + install
#   ./install.sh --uninstall   # remove installed files
# ─────────────────────────────────────────────────────────────

set -euo pipefail

APP_NAME="omarchy-settings"
BINARY_NAME="hyprland-settings"
DISPLAY_NAME="System Settings"
INSTALL_BIN="$HOME/.local/bin"
INSTALL_ICONS="$HOME/.local/share/icons/hicolor"
INSTALL_DESKTOP="$HOME/.local/share/applications"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${BLUE}→${NC} $*"; }
success() { echo -e "${GREEN}✓${NC} $*"; }
warn()    { echo -e "${YELLOW}⚠${NC} $*"; }
error()   { echo -e "${RED}✗${NC} $*"; exit 1; }

# ── Uninstall ──────────────────────────────────────────────────
if [[ "${1:-}" == "--uninstall" ]]; then
    info "Uninstalling $DISPLAY_NAME..."
    rm -f "$INSTALL_BIN/$APP_NAME"
    rm -f "$INSTALL_DESKTOP/$APP_NAME.desktop"
    rm -f "$INSTALL_ICONS/512x512/apps/$APP_NAME.png"
    rm -f "$INSTALL_ICONS/256x256/apps/$APP_NAME.png"
    rm -f "$INSTALL_ICONS/128x128/apps/$APP_NAME.png"
    rm -f "$INSTALL_ICONS/48x48/apps/$APP_NAME.png"
    gtk-update-icon-cache "$INSTALL_ICONS" 2>/dev/null || true
    update-desktop-database "$INSTALL_DESKTOP" 2>/dev/null || true
    success "Uninstalled successfully."
    exit 0
fi

echo ""
echo -e "  ${BLUE}⚙  Omarchy System Settings — Installer${NC}"
echo "  ─────────────────────────────────────"
echo ""

# ── 1. Build release binary ────────────────────────────────────
info "Building release binary (this may take a minute)..."
cd "$SCRIPT_DIR"
cargo build --release 2>&1 | grep -E "error|warning: unused|Compiling|Finished|error\[" || true

BINARY_PATH="$SCRIPT_DIR/target/release/$BINARY_NAME"
[[ -f "$BINARY_PATH" ]] || error "Build failed — binary not found at $BINARY_PATH"
success "Build complete."

# ── 2. Install binary ──────────────────────────────────────────
info "Installing binary to $INSTALL_BIN/$APP_NAME ..."
mkdir -p "$INSTALL_BIN"
cp "$BINARY_PATH" "$INSTALL_BIN/$APP_NAME"
chmod +x "$INSTALL_BIN/$APP_NAME"
success "Binary installed."

# ── 3. Install icons ───────────────────────────────────────────
info "Installing icons..."
ICON_SRC="$SCRIPT_DIR/assets/$APP_NAME.png"

if command -v convert &>/dev/null && [[ -f "$ICON_SRC" ]]; then
    for SIZE in 48 128 256 512; do
        ICON_DIR="$INSTALL_ICONS/${SIZE}x${SIZE}/apps"
        mkdir -p "$ICON_DIR"
        convert -resize "${SIZE}x${SIZE}" "$ICON_SRC" "$ICON_DIR/$APP_NAME.png" 2>/dev/null
    done
    success "Icons installed (48×48, 128×128, 256×256, 512×512)."
elif [[ -f "$ICON_SRC" ]]; then
    # Fallback: just copy at full size if ImageMagick not available
    for SIZE in 48 128 256 512; do
        ICON_DIR="$INSTALL_ICONS/${SIZE}x${SIZE}/apps"
        mkdir -p "$ICON_DIR"
        cp "$ICON_SRC" "$ICON_DIR/$APP_NAME.png"
    done
    success "Icons installed (full size, install imagemagick for proper resizing)."
else
    warn "Icon source not found — skipping icon install."
fi

# Update icon cache
gtk-update-icon-cache "$INSTALL_ICONS" 2>/dev/null || true

# ── 4. Install .desktop file ───────────────────────────────────
info "Installing .desktop entry..."
mkdir -p "$INSTALL_DESKTOP"
cat > "$INSTALL_DESKTOP/$APP_NAME.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=System Settings
GenericName=System Settings
Comment=Configure your Omarchy system settings
Exec=$INSTALL_BIN/$APP_NAME
Icon=$APP_NAME
Terminal=false
StartupNotify=true
Categories=Settings;System;
Keywords=settings;system;preferences;hyprland;omarchy;wifi;bluetooth;display;sound;audio;network;power;battery;
StartupWMClass=hyprland-settings
EOF

chmod +x "$INSTALL_DESKTOP/$APP_NAME.desktop"
update-desktop-database "$INSTALL_DESKTOP" 2>/dev/null || true
success ".desktop entry installed."

# ── 5. Verify $PATH ────────────────────────────────────────────
if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
    echo ""
    warn "\$HOME/.local/bin is not in your PATH."
    echo "   Add this to your ~/.zshrc or ~/.bashrc:"
    echo "   export PATH=\"\$HOME/.local/bin:\$PATH\""
    echo "   Then run: source ~/.zshrc"
fi

echo ""
echo -e "  ${GREEN}✓ Installation complete!${NC}"
echo ""
echo "  Launch from your app launcher: search 'System Settings'"
echo "  Or run from terminal: omarchy-settings"
echo ""
