# Platform Support Matrix

## 1. Overview
This document tracks platform-specific support status across Windows, Linux, and macOS. All platform interactions are encapsulated within platform adapters (`adapters/system/windows/`, `adapters/system/linux/`, `adapters/system/macos/`, and `adapters/system/mock/`).

## 2. Matrix Table

| Capability | Windows | Linux | macOS | Permission | Risk Level |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `system.application.open` | Supported | Supported | Supported | `system.application.open` | Medium |
| `system.application.close` | Supported | Supported | Supported | `system.application.close` | Medium |
| `system.application.focus` | Supported | Supported | Supported | `system.application.focus` | Low |
| `system.process.list` | Supported | Supported | Supported | None | Low |
| `system.process.stop` | Supported | Supported | Supported | `system.process.stop` | Medium |
| `system.process.terminate` | Supported | Supported | Supported | `system.process.terminate` | High |
| `filesystem.read` | Supported | Supported | Supported | `filesystem.read` | Low |
| `filesystem.write` | Supported | Supported | Supported | `filesystem.write` | Medium |
| `filesystem.delete` | Supported | Supported | Supported | `filesystem.delete` | High |
| `terminal.execute` | Supported | Supported | Supported | `terminal.execute` | High |
| `browser.open` | Supported | Supported | Supported | `browser.access` | Low |
| `browser.navigate` | Supported | Supported | Supported | `browser.access` | Low |
| `window.list` | Supported | Supported | Supported | None | Low |
| `window.focus` | Supported | Supported | Supported | `window.control` | Low |
| `window.resize` | Supported | Supported | Supported | `window.control` | Low |
| `input.key.press` | Supported | Supported | Supported | `input.inject` | High |
| `input.mouse.click` | Supported | Supported | Supported | `input.inject` | High |
| `clipboard.read` | Supported | Supported | Supported | `clipboard.read` | Medium |
| `clipboard.write` | Supported | Supported | Supported | `clipboard.write` | Low |
| `screen.capture` | Supported | Supported | Supported | `screen.capture` | Medium |
| `display.list` | Supported | Supported | Supported | None | Low |
| `display.brightness` | Supported | Partial | Supported | `display.control` | Low |
| `media.volume` | Supported | Supported | Supported | `media.control` | Low |
| `system.get_state` | Supported | Supported | Supported | None | Low |
| `system.lock` | Supported | Supported | Supported | `system.power` | Medium |
| `system.shutdown` | Supported | Supported | Supported | `system.power` (Confirmed) | Critical |
| `system.notification.send` | Supported | Supported | Supported | `notification.send` | Low |
