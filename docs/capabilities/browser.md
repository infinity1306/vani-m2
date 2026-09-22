# Browser Capability Specification

## 1. Overview
The Browser Capability abstracts web browser automation behind provider-neutral interfaces with strict separation between read-only and state-changing actions.

## 2. Capabilities
- **Read-Only (Low Risk)**:
  - `browser.open`: Launches a browser session.
  - `browser.navigate`: Navigates to a specific URL.
  - `browser.search`: Queries a search engine.
  - `browser.get_page`: Retrieves HTML/DOM page content.
  - `browser.extract`: Extracts text matching a selector.
  - `browser.screenshot`: Captures a screenshot of the browser tab.
- **State-Changing (Medium Risk / Guarded)**:
  - `browser.click`: Clicks an element.
  - `browser.type`: Types text into an input element.
  - `browser.select`: Selects a dropdown option.
  - `browser.submit_form`: Submits a form.

## 3. Session Model
Every browser interaction is scoped to a `BrowserSession` (`id`, `profile`, `tabs`, `active_tab_id`, `task_id`).
