#!/usr/bin/env bash
set -e

REPO="bebekgalaksi/MasterDuel-Switcher"
INSTALL_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/128x128/apps"
TAR_URL="https://github.com/$REPO/releases/latest/download/MasterDuelSwitcher-Linux-x86_64.tar.gz"

echo "=== Installing Master Duel Switcher ==="

mkdir -p "$INSTALL_DIR"
mkdir -p "$DESKTOP_DIR"

# If pre-compiled binary is available locally, use it; otherwise download latest release
if [ -f "build/MasterDuelSwitcher" ]; then
    cp build/MasterDuelSwitcher "$INSTALL_DIR/MasterDuelSwitcher"
else
    echo "Downloading latest release..."
    TMP_DIR=$(mktemp -d)
    curl -sSL "$TAR_URL" -o "$TMP_DIR/switcher.tar.gz"
    tar -xzf "$TMP_DIR/switcher.tar.gz" -C "$TMP_DIR"
    cp "$TMP_DIR/MasterDuelSwitcher" "$INSTALL_DIR/MasterDuelSwitcher"
    rm -rf "$TMP_DIR"
fi

chmod +x "$INSTALL_DIR/MasterDuelSwitcher"

# Create Desktop Entry
cat <<EOF > "$DESKTOP_DIR/masterduel-switcher.desktop"
[Desktop Entry]
Type=Application
Name=Master Duel Switcher
Comment=Switch Master Duel accounts without redownloading 10GB
Exec=$INSTALL_DIR/MasterDuelSwitcher
Icon=steam_icon_1449850
Terminal=false
Categories=Game;Utility;
Keywords=yugioh;master;duel;md;switcher;account;steam;
StartupNotify=false
EOF

chmod +x "$DESKTOP_DIR/masterduel-switcher.desktop"

echo ""
echo "✓ Installed successfully to $INSTALL_DIR/MasterDuelSwitcher"
echo "✓ Desktop launcher added: available in your application menu."
