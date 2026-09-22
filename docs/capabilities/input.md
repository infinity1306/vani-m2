# Input Capability Specification

## 1. Overview
The Input Capability abstracts keyboard and mouse events. All injection is permission-guarded and context-targeted.

## 2. Capabilities
- `input.key.press`: Injects a single key press (e.g. `Enter`, `Escape`, `Ctrl+C`).
- `input.key.type`: Injects a sequence of keystrokes.
- `input.mouse.click`: Injects a mouse click at coordinates (x, y) with specified button.
- `input.mouse.move`: Moves mouse cursor to coordinates (x, y).
- `input.mouse.scroll`: Injects vertical scroll deltas.

## 3. Targeting
All input actions accept an `InputTargetContext` specifying target application, window, monitor, or browser tab.
