// SPDX-License-Identifier: GPL-3.0-only
//
// ClaudeUIControl: a small helper app that lets Claude drive Dorico 6 and this fork's MuseScore build (keys, clicks,
// drags, typing, window bounds) to compare and test the two UIs. See README.md next to this file.
//
// Safety rules, enforced below:
//   - Only two apps can be controlled, identified by bundle ID (ALLOWED_APPS). Keys go only to an allowed app that is
//     frontmost; clicks, drags and moves only land inside a window of an allowed, frontmost app (its main window or one
//     of its popups); anything else is refused and nothing is sent.
//   - It never reads keystrokes, screen contents or files. It only sends input and reads window positions.
//   - It is its own app (started with `open`), so it has its own Accessibility permission, which the user grants and
//     can switch off in System Settings. It is ad-hoc signed: any change to the binary makes macOS drop the permission.
//
// Commands (via uictl.sh; chain several with ";" as a separate argument):
//   trusted                      prints true/false: is Accessibility granted?
//   prompt                       asks macOS to list this app under Accessibility (switch it on there)
//   front <app>                  bring an allowed app to the front          (<app>: dorico | mscore)
//   bounds <app>                 x,y,w,h of its main (largest) window, screen points, top-left origin
//   key <app> <combo>            front <app>, then press e.g. cmd+1, shift+d, return, escape (US layout)
//   type <text>                  type text into the frontmost allowed app
//   click <x> <y> [right|double|cmd|shift|alt|ctrl ...]
//   drag <x1> <y1> <x2> <y2>
//   move <x> <y>
//   wait <ms>

import AppKit
import ApplicationServices

let ALLOWED_APPS: [String: String] = [
    "dorico": "com.steinberg.dorico6",
    "mscore": "org.musescore.MuseScoreStudio5Development",
]

func out(_ s: String) {
    print(s)
    fflush(stdout)
}

func fail(_ s: String) {
    out("error: " + s)
}

func isAllowed(_ app: NSRunningApplication?) -> Bool {
    guard let id = app?.bundleIdentifier else { return false }
    return ALLOWED_APPS.values.contains(id)
}

func allowedApp(_ name: String) -> NSRunningApplication? {
    guard let bundleId = ALLOWED_APPS[name.lowercased()] else {
        fail("'\(name)' is not an allowed app (allowed: \(ALLOWED_APPS.keys.sorted().joined(separator: ", ")))")
        return nil
    }
    guard let app = NSRunningApplication.runningApplications(withBundleIdentifier: bundleId).first else {
        fail("\(name) is not running")
        return nil
    }
    return app
}

func frame(of window: AXUIElement) -> CGRect {
    var posRef: CFTypeRef?
    var sizeRef: CFTypeRef?
    AXUIElementCopyAttributeValue(window, kAXPositionAttribute as CFString, &posRef)
    AXUIElementCopyAttributeValue(window, kAXSizeAttribute as CFString, &sizeRef)
    var pos = CGPoint.zero
    var size = CGSize.zero
    if let p = posRef { AXValueGetValue(p as! AXValue, .cgPoint, &pos) }
    if let s = sizeRef { AXValueGetValue(s as! AXValue, .cgSize, &size) }
    return CGRect(origin: pos, size: size)
}

// All windows of the app (main window, popups, dialogs)
func windowFrames(of app: NSRunningApplication) -> [CGRect] {
    let el = AXUIElementCreateApplication(app.processIdentifier)
    var value: CFTypeRef?
    guard AXUIElementCopyAttributeValue(el, kAXWindowsAttribute as CFString, &value) == .success,
          let list = value as? [AXUIElement] else { return [] }
    return list.map(frame(of:))
}

// The largest window: the app's main window even while a popup has focus
func mainWindowFrame(of app: NSRunningApplication) -> CGRect? {
    windowFrames(of: app).max { $0.width * $0.height < $1.width * $1.height }
}

func frontWindowFrame(of app: NSRunningApplication) -> CGRect? {
    let el = AXUIElementCreateApplication(app.processIdentifier)
    var value: CFTypeRef?
    var window: AXUIElement?
    if AXUIElementCopyAttributeValue(el, kAXFocusedWindowAttribute as CFString, &value) == .success, let v = value {
        window = (v as! AXUIElement)
    } else if AXUIElementCopyAttributeValue(el, kAXWindowsAttribute as CFString, &value) == .success,
              let list = value as? [AXUIElement], let first = list.first {
        window = first
    }
    guard let w = window else { return nil }
    var posRef: CFTypeRef?
    var sizeRef: CFTypeRef?
    AXUIElementCopyAttributeValue(w, kAXPositionAttribute as CFString, &posRef)
    AXUIElementCopyAttributeValue(w, kAXSizeAttribute as CFString, &sizeRef)
    var pos = CGPoint.zero
    var size = CGSize.zero
    if let p = posRef { AXValueGetValue(p as! AXValue, .cgPoint, &pos) }
    if let s = sizeRef { AXValueGetValue(s as! AXValue, .cgSize, &size) }
    return CGRect(origin: pos, size: size)
}

// The gate for every input event: the frontmost app must be allowed, and a pointer event must land in its window.
func inputAllowed(at point: CGPoint? = nil) -> Bool {
    let frontApp = NSWorkspace.shared.frontmostApplication
    guard isAllowed(frontApp), let app = frontApp else {
        fail("refused: the frontmost app (\(frontApp?.localizedName ?? "none")) is not an allowed app")
        return false
    }
    if let p = point {
        guard windowFrames(of: app).contains(where: { $0.contains(p) }) else {
            fail("refused: (\(Int(p.x)), \(Int(p.y))) is outside \(app.localizedName ?? "the app")'s windows")
            return false
        }
    }
    return true
}

func front(_ name: String) -> Bool {
    guard let app = allowedApp(name) else { return false }
    let el = AXUIElementCreateApplication(app.processIdentifier)
    AXUIElementSetAttributeValue(el, kAXFrontmostAttribute as CFString, kCFBooleanTrue)
    app.activate()
    for _ in 0..<20 where NSWorkspace.shared.frontmostApplication?.processIdentifier != app.processIdentifier {
        usleep(100_000)
    }
    usleep(200_000)
    return NSWorkspace.shared.frontmostApplication?.processIdentifier == app.processIdentifier
}

// US-layout virtual key codes
let keyCodes: [String: CGKeyCode] = [
    "a": 0, "s": 1, "d": 2, "f": 3, "h": 4, "g": 5, "z": 6, "x": 7, "c": 8, "v": 9, "b": 11, "q": 12, "w": 13,
    "e": 14, "r": 15, "y": 16, "t": 17, "1": 18, "2": 19, "3": 20, "4": 21, "6": 22, "5": 23, "=": 24, "9": 25,
    "7": 26, "-": 27, "8": 28, "0": 29, "]": 30, "o": 31, "u": 32, "[": 33, "i": 34, "p": 35, "return": 36,
    "l": 37, "j": 38, "'": 39, "k": 40, ";": 41, "\\": 42, ",": 43, "/": 44, "n": 45, "m": 46, ".": 47,
    "tab": 48, "space": 49, "`": 50, "delete": 51, "backspace": 51, "escape": 53, "esc": 53,
    "forwarddelete": 117, "home": 115, "end": 119, "pageup": 116, "pagedown": 121,
    "left": 123, "right": 124, "down": 125, "up": 126,
    "f1": 122, "f2": 120, "f3": 99, "f4": 118, "f5": 96, "f6": 97, "f7": 98, "f8": 100, "f9": 101, "f10": 109,
    "f11": 103, "f12": 111,
]

func modifierFlags(_ names: [String]) -> CGEventFlags {
    var flags: CGEventFlags = []
    for m in names {
        switch m {
        case "cmd", "command": flags.insert(.maskCommand)
        case "shift": flags.insert(.maskShift)
        case "alt", "opt", "option": flags.insert(.maskAlternate)
        case "ctrl", "control": flags.insert(.maskControl)
        default: break
        }
    }
    return flags
}

func key(_ combo: String) {
    let parts = combo.lowercased().split(separator: "+").map(String.init)
    guard let last = parts.last, let code = keyCodes[last] else { fail("unknown key '\(combo)'"); return }
    guard inputAllowed() else { return }
    let flags = modifierFlags(Array(parts.dropLast()))
    for down in [true, false] {
        let e = CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: down)
        e?.flags = flags
        e?.post(tap: .cghidEventTap)
        usleep(20_000)
    }
}

func type(_ text: String) {
    guard inputAllowed() else { return }
    for ch in text.utf16 {
        var c = ch
        for down in [true, false] {
            let e = CGEvent(keyboardEventSource: nil, virtualKey: 0, keyDown: down)
            e?.keyboardSetUnicodeString(stringLength: 1, unicodeString: &c)
            e?.post(tap: .cghidEventTap)
        }
        usleep(15_000)
    }
}

func mouse(_ type: CGEventType, _ p: CGPoint, _ button: CGMouseButton, flags: CGEventFlags = [], clicks: Int64 = 1) {
    let e = CGEvent(mouseEventSource: nil, mouseType: type, mouseCursorPosition: p, mouseButton: button)
    e?.flags = flags
    e?.setIntegerValueField(.mouseEventClickState, value: clicks)
    e?.post(tap: .cghidEventTap)
}

// Press or release the modifier keys themselves: apps such as Qt read the keyboard state, not only the click's flags
let modifierKeyCodes: [String: CGKeyCode] = ["cmd": 55, "command": 55, "shift": 56, "alt": 58, "opt": 58, "option": 58,
                                             "ctrl": 59, "control": 59]

func pressModifiers(_ names: [String], down: Bool) {
    var flags: CGEventFlags = down ? [] : modifierFlags(names)
    for name in names {
        guard let code = modifierKeyCodes[name] else { continue }
        if down { flags.formUnion(modifierFlags([name])) } else { flags.subtract(modifierFlags([name])) }
        let e = CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: down)
        e?.flags = flags
        e?.post(tap: .cghidEventTap)
        usleep(20_000)
    }
}

func click(_ p: CGPoint, _ opts: [String]) {
    guard inputAllowed(at: p) else { return }
    let flags = modifierFlags(opts)
    let modifiers = opts.filter { modifierKeyCodes[$0] != nil }
    pressModifiers(modifiers, down: true)
    defer { pressModifiers(modifiers, down: false) }
    mouse(.mouseMoved, p, .left)
    usleep(50_000)
    if opts.contains("right") {
        mouse(.rightMouseDown, p, .right, flags: flags)
        mouse(.rightMouseUp, p, .right, flags: flags)
        return
    }
    let count: Int64 = opts.contains("double") ? 2 : 1
    for n in 1...count {
        mouse(.leftMouseDown, p, .left, flags: flags, clicks: n)
        mouse(.leftMouseUp, p, .left, flags: flags, clicks: n)
        usleep(60_000)
    }
}

func drag(_ a: CGPoint, _ b: CGPoint) {
    guard inputAllowed(at: a), inputAllowed(at: b) else { return }
    mouse(.mouseMoved, a, .left)
    mouse(.leftMouseDown, a, .left)
    usleep(150_000)
    let steps = 20
    for i in 1...steps {
        let t = Double(i) / Double(steps)
        mouse(.leftMouseDragged, CGPoint(x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t), .left)
        usleep(25_000)
    }
    usleep(150_000)
    mouse(.leftMouseUp, b, .left)
}

func point(_ x: String, _ y: String) -> CGPoint {
    CGPoint(x: Double(x) ?? -1, y: Double(y) ?? -1)
}

func run(_ cmd: [String]) {
    guard let name = cmd.first else { return }
    let a = Array(cmd.dropFirst())
    if !["trusted", "prompt", "wait"].contains(name) && !AXIsProcessTrusted() {
        fail("Accessibility is not granted to ClaudeUIControl (System Settings > Privacy & Security > Accessibility)")
        return
    }
    switch name {
    case "trusted":
        out(AXIsProcessTrusted() ? "true" : "false")
    case "prompt":
        let opts = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: true] as CFDictionary
        out(AXIsProcessTrustedWithOptions(opts) ? "true" : "false")
    case "front" where a.count == 1:
        if !front(a[0]) { fail("could not bring \(a[0]) to the front") }
    case "bounds" where a.count == 1:
        if let app = allowedApp(a[0]) {
            if let f = mainWindowFrame(of: app) ?? frontWindowFrame(of: app) {
                out("\(Int(f.minX)),\(Int(f.minY)),\(Int(f.width)),\(Int(f.height))")
            } else {
                fail("\(a[0]) has no window")
            }
        }
    case "key" where a.count == 2:
        if front(a[0]) { key(a[1]) } else { fail("could not bring \(a[0]) to the front") }
    case "type" where a.count >= 1:
        type(a.joined(separator: " "))
    case "click" where a.count >= 2:
        click(point(a[0], a[1]), Array(a.dropFirst(2)))
    case "drag" where a.count == 4:
        drag(point(a[0], a[1]), point(a[2], a[3]))
    case "move" where a.count == 2:
        let p = point(a[0], a[1])
        if inputAllowed(at: p) { mouse(.mouseMoved, p, .left) }
    case "wait" where a.count == 1:
        usleep(useconds_t(min(Int(a[0]) ?? 0, 10_000) * 1000))
    default:
        fail("bad command: \(cmd.joined(separator: " "))")
    }
}

var current: [String] = []
for arg in CommandLine.arguments.dropFirst() {
    if arg == ";" {
        run(current)
        current = []
    } else {
        current.append(arg)
    }
}
run(current)
