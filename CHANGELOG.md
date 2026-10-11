# Changelog

## 0.1.0 (2026-10-11)


### ⚠ BREAKING CHANGES

* VogueMenuItem.disabled is now VogueMenuItem.flags. Use VOGUE_MENU_ITEM_DISABLED to disable an item and VOGUE_MENU_ITEM_KEEP_OPEN to keep its menu open after activation. Zero flags keeps the existing default.
* VogueAccessibilityNode adds a controls relation. Rebuild consumers and update foreign-function bindings to match the new structure layout.
* include <vogue.h> instead of <vogue_accessibility.h>.

### Features

* add headless command menus ([d2c5062](https://github.com/chiefGui/vogue/commit/d2c5062959e655d0e6c9727f2db9fd7ee0a5d274))
* add menu item keep-open flags ([71840aa](https://github.com/chiefGui/vogue/commit/71840aa9ce96bee721f88b60d23b7f693be426fc))


### Miscellaneous Chores

* **ci:** set up Release Please ([eb4c037](https://github.com/chiefGui/vogue/commit/eb4c037d9a38f3294b3e2db32157c368509b33d9))


### Code Refactoring

* consolidate public API in vogue.h ([ad58ebe](https://github.com/chiefGui/vogue/commit/ad58ebe80ee35b002cb04c36c2e52900be832c1a))
