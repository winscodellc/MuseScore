#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# Builds ~/Applications/ClaudeUIControl.app from main.swift and signs it ad hoc. Run it yourself (it is not run by
# Claude). After the first build, switch the app on in System Settings > Privacy & Security > Accessibility.
# Rebuilding changes the signature, so macOS drops the permission and it must be switched on again.
set -e
here=$(cd "$(dirname "$0")" && pwd)
app="$HOME/Applications/ClaudeUIControl.app"

mkdir -p "$app/Contents/MacOS"
cat > "$app/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key><string>com.winsvu.ClaudeUIControl</string>
    <key>CFBundleName</key><string>ClaudeUIControl</string>
    <key>CFBundleExecutable</key><string>ClaudeUIControl</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>1.0</string>
    <key>LSUIElement</key><true/>
</dict>
</plist>
PLIST

swiftc -O "$here/main.swift" -o "$app/Contents/MacOS/ClaudeUIControl"
codesign --force --sign - --identifier com.winsvu.ClaudeUIControl "$app"
echo "Built $app"
echo "Next: System Settings > Privacy & Security > Accessibility > + > $app, and switch it on."
