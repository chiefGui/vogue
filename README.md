# Vogue

A renderer-independent C11 library for UI behavior. The initial core provides
contexts and buttons through a C ABI, with no third-party dependencies.

The host owns layout, hit testing, event routing, drawing, focus order, and
platform integration. Vogue owns interaction transitions and returns state and
host requests. Native accessibility integration and other components are future
work.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The default is a static library. Use `-DBUILD_SHARED_LIBS=ON` for a shared library.
Multi-configuration generators select optimization with `--config Release`.
Vogue does not override the consuming application's compiler or optimization
flags.

To consume a checkout or Git submodule from an existing CMake application:

```cmake
add_subdirectory(vendor/vogue)
target_link_libraries(your_app PRIVATE vogue::vogue)
```

Or compile `src/context.c` and `src/button.c` with `include/` on your include path.
Include `<vogue.h>` from application code. Bindings can use the exported C API;
there are no language wrappers yet. The initial API and struct layout may change;
rebuild consumers against the same revision as the library.

## Ownership and cost

- The application owns `VogueContext` and `VogueButton` storage. Stack allocation
  and arrays are supported. Initialize objects with their respective functions.
  On the verified x64 builds, a context occupies 40 bytes and a button 16 bytes.
- Component IDs are host-supplied nonzero integers, unique within a context.
  Associate each live button with one context. Do not mutate the private fields.
  Duplicate IDs are host errors; the core does not scan for them.
- All current operations are O(1) in the number of components. There are no
  internal allocations, component registries, string hashing, traversal, locks,
  or per-frame rebuilds. Events go directly to the target button.
- No host pointers or event pointers are retained. Each context is independently
  owned and used from one thread at a time.
- Each context supports one focused component, one hovered component, and one
  active gesture. Simultaneous gestures in the same context are ignored.
- Initialization is for fresh storage. Before removing a component or reusing
  its ID, call `vogue_context_forget` and apply the returned effects. Before
  disposing of a live context, call `vogue_context_reset` and apply its effects.

These are implementation cost guarantees, not measured end-to-end UI performance.
Layout, routing, rendering, and host services have their own costs.

## Host contract

1. Normalize native input into `VogueEvent`. Vogue key/button constants are not
   operating-system codes. Handle application shortcuts before delivery.
2. Hit-test in the host. Set `VOGUE_EVENT_INSIDE` from the current interactive
   region, including clipping, whenever delivering pointer move/down/up events.
   On hover changes without capture, deliver an outside move to the previous
   target and an inside move to the new target. Deliver an outside move when the
   pointer leaves the window. Update hover after geometry changes as needed.
3. Apply capture requests and route subsequent pointer events to
   `vogue_context_captured`, even outside its bounds. Preserve pointer IDs.
   After release, refresh hover from the current pointer position. If capture
   fails or is lost, deliver `POINTER_CANCEL` to its owner with the matching ID.
4. Choose enabled focus targets with `vogue_button_focus` and route keyboard
   events to `vogue_context_focused`. Keep native focus synchronized. When the
   window loses focus, call `vogue_context_reset`. On native focus changes outside
   this context, reset it; on changes to another Vogue button, focus that button.
5. Apply every returned `VogueEffects` before the next input event. Release old
   pointer capture before transferring focus; acquire new capture afterward.
   Apply activation last, since application actions may remove components.
   A zero focus target means clear host logical focus. Already-applied native
   focus/capture changes can be acknowledged without repeating the platform call.
6. Read `vogue_button_state` to style the UI. Focus and capture changes may affect
   a previously focused/pressed component as well as the current target.

If focus or capture cannot be established, cancel/reset the affected interaction
before delivering further input. Reconcile stale hover through the same pointer
move protocol after reset, removal, or re-enabling a component.

## Button behavior

- A primary pointer press inside focuses and captures. Release from that pointer
  activates only when inside; release outside cancels activation.
- Leaving while pressed clears the pressed appearance. Re-entering restores it.
  Other pointer IDs and non-primary button releases cannot finish the gesture.
- Enter activates on key down; Space activates on key up. Repeated key downs
  do not activate again. Other key releases cannot complete the gesture.
- Cancellation, focus transfer, reset, removal, and disabling end the gesture
  without activation. Disabling also clears the button's focus and hover.

```c
VogueContext ui;
VogueButton save;
vogue_context_init(&ui);
vogue_button_init(&save, UINT64_C(1));

/* Inside your host's event dispatch, after routing and hit testing: */
VogueEvent event = {
    .type = VOGUE_EVENT_POINTER_DOWN,
    .pointer_id = 0,
    .button = VOGUE_POINTER_PRIMARY,
    .flags = VOGUE_EVENT_INSIDE
};
VogueEffects effects = vogue_button_handle(&ui, &save, &event);
/* Apply effects to host focus/capture and handle ACTIVATE, then read state. */
uint32_t state = vogue_button_state(&ui, &save);
```

Objects need no per-frame update when input and configuration are unchanged.
