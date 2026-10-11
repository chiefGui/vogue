#ifndef VOGUE_H
#define VOGUE_H

#include <stdint.h>

#if defined(VOGUE_SHARED) && defined(_WIN32)
#  if defined(VOGUE_BUILD)
#    define VOGUE_API __declspec(dllexport)
#  else
#    define VOGUE_API __declspec(dllimport)
#  endif
#elif defined(VOGUE_SHARED) && defined(__GNUC__)
#  define VOGUE_API __attribute__((visibility("default")))
#else
#  define VOGUE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* IDs are supplied by the host, unique within a context, and never zero. */
typedef uint64_t VogueId;
#define VOGUE_ID_NONE UINT64_C(0)

/* Constants use fixed-width fields below, rather than ABI-dependent enum fields. */
enum {
    VOGUE_EVENT_POINTER_MOVE = 1,
    VOGUE_EVENT_POINTER_DOWN,
    VOGUE_EVENT_POINTER_UP,
    VOGUE_EVENT_POINTER_CANCEL,
    VOGUE_EVENT_KEY_DOWN,
    VOGUE_EVENT_KEY_UP
};

enum {
    VOGUE_KEY_NONE = 0,
    VOGUE_KEY_ENTER,
    VOGUE_KEY_SPACE,
    VOGUE_KEY_UP,
    VOGUE_KEY_DOWN,
    VOGUE_KEY_HOME,
    VOGUE_KEY_END,
    VOGUE_KEY_ESCAPE,
    VOGUE_KEY_TAB
};

enum {
    VOGUE_POINTER_PRIMARY = 0,
    VOGUE_POINTER_SECONDARY,
    VOGUE_POINTER_MIDDLE
};

enum {
    /* The pointer is inside the target's current interactive region. */
    VOGUE_EVENT_INSIDE = 1u << 0,
    VOGUE_EVENT_REPEAT = 1u << 1,
    VOGUE_EVENT_SHIFT = 1u << 2
};

typedef struct VogueEvent {
    uint32_t type;
    uint32_t pointer_id; /* Zero is a valid pointer ID. */
    uint32_t button;     /* Used by POINTER_DOWN and POINTER_UP. */
    uint32_t key;        /* Used by KEY_DOWN and KEY_UP. */
    uint32_t flags;
} VogueEvent;

enum {
    VOGUE_STATE_HOVERED = 1u << 0,
    VOGUE_STATE_PRESSED = 1u << 1,
    VOGUE_STATE_FOCUSED = 1u << 2,
    VOGUE_STATE_DISABLED = 1u << 3
};

enum {
    VOGUE_EFFECT_ACTIVATE = 1u << 0,
    VOGUE_EFFECT_FOCUS = 1u << 1,
    VOGUE_EFFECT_CAPTURE_POINTER = 1u << 2,
    VOGUE_EFFECT_RELEASE_POINTER = 1u << 3,
    /* Re-query the affected control's semantics. Focus uses EFFECT_FOCUS. */
    VOGUE_EFFECT_SEMANTICS_CHANGED = 1u << 4
};

/* Apply effects before the next event: release capture, move focus, acquire
 * capture, then activate. Button ACTIVATE belongs to the button being handled;
 * menus report the activated item in VogueMenuEffects.
 * A FOCUS effect with focus_target == VOGUE_ID_NONE clears logical host focus.
 * Fields without their corresponding flag are unspecified and must be ignored.
 */
typedef struct VogueEffects {
    uint32_t flags;
    uint32_t pointer_id;
    VogueId pointer_target;
    VogueId focus_target;
} VogueEffects;

/* Caller-owned storage. Initialize through the API; treat fields as private.
 * A context has one focus, one hover, and at most one active gesture.
 * Use a context from one thread at a time. There are no allocations or locks.
 */
typedef struct VogueContext {
    VogueId _focused;
    VogueId _hovered;
    VogueId _active;
    uint32_t _pointer;
    uint32_t _press;
    uint32_t _inside;
} VogueContext;

typedef struct VogueButton {
    VogueId _id;
    uint32_t _disabled;
} VogueButton;

/* Unless documented otherwise, pointers must be non-null and refer to
 * initialized objects; init functions accept uninitialized output storage.
 * Debug assertions check null pointers and zero component IDs. Object lifetime,
 * initialization, and ID uniqueness remain the host's responsibility.
 */
VOGUE_API void vogue_context_init(VogueContext *context);

/* Cancel interactions and clear focus/hover, e.g. when the window loses focus.
 * Consume the returned effects. Existing button objects remain usable.
 */
VOGUE_API VogueEffects vogue_context_reset(VogueContext *context);

/* Call before removing a component or reusing its ID. */
VOGUE_API VogueEffects vogue_context_forget(VogueContext *context, VogueId id);
VOGUE_API VogueId vogue_context_focused(const VogueContext *context);
VOGUE_API VogueId vogue_context_captured(const VogueContext *context);

VOGUE_API void vogue_button_init(VogueButton *button, VogueId id);

/* Disabling cancels an active gesture and clears this button's focus/hover. */
VOGUE_API VogueEffects vogue_button_set_disabled(
    VogueContext *context, VogueButton *button, uint32_t disabled);

/* The host chooses focus order. A disabled button cannot receive focus.
 * Moving focus cancels any gesture belonging to the previous focus.
 */
VOGUE_API VogueEffects vogue_button_focus(
    VogueContext *context, const VogueButton *button);

/* The host hit-tests and sets INSIDE on pointer move/down/up events.
 * On hover changes, send an outside move to the old target, then an inside move
 * to the new one. During capture, route pointer events to its owner even outside;
 * refresh hover after release, and send POINTER_CANCEL if capture is lost.
 * Send keyboard events to the focused button.
 */
VOGUE_API VogueEffects vogue_button_handle(
    VogueContext *context, const VogueButton *button, const VogueEvent *event);

/* Read current state without mutation. This performs no allocation or lookup. */
VOGUE_API uint32_t vogue_button_state(
    const VogueContext *context, const VogueButton *button);

#define VOGUE_MENU_ITEM_NONE UINT32_MAX

enum {
    VOGUE_MENU_ITEM_DISABLED = 1u << 0,
    /* Activate without closing, for pointer, keyboard, and accessibility input. */
    VOGUE_MENU_ITEM_KEEP_OPEN = 1u << 1
};

/* Borrowed descriptors in navigation order. IDs are unique, nonzero, and distinct
 * from the menu ID. Keep storage, IDs, count, and order stable until reinitializing
 * a closed menu. The host may change flags on the context's thread; refresh that
 * item's semantics after changing DISABLED. Disabled items can focus but not activate.
 * Zero flags means enabled and close on activation. KEEP_OPEN ends the gesture
 * without closing or restoring focus; the host owns any confirmation state.
 * Include only command items; decorative gaps do not need array entries.
 */
typedef struct VogueMenuItem {
    VogueId id;
    uint32_t flags;
} VogueMenuItem;

/* Caller-owned; initialize through the API and treat fields as private. */
typedef struct VogueMenu {
    const VogueMenuItem *_items;
    VogueId _id;
    VogueId _return_focus;
    uint32_t _count;
    uint32_t _focused;
    uint32_t _hovered;
    uint32_t _open;
} VogueMenu;

enum {
    VOGUE_MENU_EFFECT_OPENED = 1u << 0,
    VOGUE_MENU_EFFECT_CLOSED = 1u << 1,
    VOGUE_MENU_EFFECT_FOCUS_NEXT = 1u << 2,
    VOGUE_MENU_EFFECT_FOCUS_PREVIOUS = 1u << 3,
    VOGUE_MENU_EFFECT_HANDLED = 1u << 4
};

/* Apply visibility changes and common effects before dispatching another event.
 * activated_index is valid only with effects.flags & VOGUE_EFFECT_ACTIVATE.
 * For FOCUS_NEXT/PREVIOUS, apply common effects first, then move to the adjacent
 * host tab stop. HANDLED means consume the event, including Tab (no default Tab).
 * OPENED/CLOSED also invalidate the trigger's expanded accessibility state.
 */
typedef struct VogueMenuEffects {
    VogueEffects effects;
    uint32_t flags;
    uint32_t activated_index;
} VogueMenuEffects;

/* items may be null only when count is zero. Initialization does not copy items. */
VOGUE_API void vogue_menu_init(
    VogueMenu *menu, VogueId id, const VogueMenuItem *items, uint32_t count);

/* A VogueButton can trigger the menu: on ACTIVATE, close if open, otherwise open.
 * Route trigger events separately from outside dismissal, and focus the trigger
 * before opening if activation did not focus it (e.g. an accessibility INVOKE).
 * Do not dispatch that same event to the newly opened menu. initial_index selects
 * the first focus target (0 for first, count-1 for last); NONE or an empty array
 * focuses the menu itself. Saves current context focus for later restoration.
 * Opening an already open menu does nothing. Keep its return-focus target live.
 * Use the same context until closed; the host closes any prior menu before opening.
 */
VOGUE_API VogueMenuEffects vogue_menu_open(
    VogueContext *context, VogueMenu *menu, uint32_t initial_index);

/* Close before reset, removal, reinitialization, or replacing the open menu.
 * Restores saved focus only if restore_focus != 0 and the menu still owns focus;
 * otherwise clears its focus without disturbing another control's focus.
 * Use restore_focus == 0 for outside focus/clicks or a removed/disabled trigger.
 */
VOGUE_API VogueMenuEffects vogue_menu_close(
    VogueContext *context, VogueMenu *menu, uint32_t restore_focus);
VOGUE_API uint32_t vogue_menu_is_open(const VogueMenu *menu);
VOGUE_API uint32_t vogue_menu_focused(
    const VogueContext *context, const VogueMenu *menu);

/* Focus an item, including a disabled item; NONE focuses the container.
 * Does nothing when closed. Also supports host-provided typeahead matching.
 */
VOGUE_API VogueMenuEffects vogue_menu_focus(
    VogueContext *context, VogueMenu *menu, uint32_t index);

/* Route keyboard events here while open. Arrows wrap; Home/End select endpoints;
 * Enter/Space activate on non-repeated key down. Escape restores focus, as does
 * activation unless the item has KEEP_OPEN.
 * Tab closes, restores the origin, and requests host traversal in either direction.
 * For pointer events, INSIDE means inside the popup and index is the hit item or
 * NONE for empty space. Capture targets the menu ID; keep routing outside moves
 * and release/cancel events to it. A press can drag to another item and release
 * to activate it. Other pointers cannot take over a captured gesture.
 * An outside down closes without restoration and is NOT handled: apply effects,
 * then route that same down to its actual target. If logical focus has already
 * left the menu, the next handle call closes it without handling the event.
 */
VOGUE_API VogueMenuEffects vogue_menu_handle(
    VogueContext *context, VogueMenu *menu, uint32_t index,
    const VogueEvent *event);
VOGUE_API uint32_t vogue_menu_item_state(
    const VogueContext *context, const VogueMenu *menu, uint32_t index);

enum {
    VOGUE_ROLE_GROUP = 1,
    VOGUE_ROLE_BUTTON,
    VOGUE_ROLE_MENU,
    VOGUE_ROLE_MENU_ITEM
};

enum {
    VOGUE_ACCESSIBLE_FOCUSABLE = 1u << 0,
    VOGUE_ACCESSIBLE_FOCUSED = 1u << 1,
    VOGUE_ACCESSIBLE_DISABLED = 1u << 2,
    VOGUE_ACCESSIBLE_OFFSCREEN = 1u << 3,
    VOGUE_ACCESSIBLE_HAS_POPUP_MENU = 1u << 4,
    VOGUE_ACCESSIBLE_EXPANDED = 1u << 5
};

enum {
    VOGUE_ACTION_FOCUS = 1u << 0,
    VOGUE_ACTION_INVOKE = 1u << 1,
    VOGUE_ACTION_DISMISS = 1u << 2
};

/* Borrowed UTF-8, with a byte length (no terminator required). Keep data alive
 * while published. Increment revision on in-place edits so changes are detected
 * without scanning text; copy old text in the adapter if its platform needs it.
 */
typedef struct VogueAccessibleText {
    const char *data;
    uint32_t length;
    uint32_t revision;
} VogueAccessibleText;

typedef struct VogueAccessibleBounds {
    float x;
    float y;
    float width;
    float height;
} VogueAccessibleBounds;

/* Host-owned tree node. IDs share the context's namespace; zero means no link.
 * The host maintains coherent parent/child/sibling links, labels, and finite
 * screen-space bounds, converting coordinates to platform units in its adapter.
 * Exclude hidden controls from the published tree. OFFSCREEN means an exposed
 * control is outside the visible area; it can still support accessibility actions.
 */
typedef struct VogueAccessibilityNode {
    VogueId id;
    VogueId parent;
    VogueId first_child;
    VogueId last_child;
    VogueId previous_sibling;
    VogueId next_sibling;
    VogueId controls; /* Controlled node, e.g. a trigger's menu; zero means none. */
    VogueAccessibleText name;
    VogueAccessibleText description;
    VogueAccessibleBounds bounds;
    uint32_t role;
    uint32_t state;
    uint32_t actions;
} VogueAccessibilityNode;

enum {
    VOGUE_ACCESSIBILITY_ADDED = 1u << 0,
    VOGUE_ACCESSIBILITY_REMOVED = 1u << 1,
    VOGUE_ACCESSIBILITY_NAME_CHANGED = 1u << 2,
    VOGUE_ACCESSIBILITY_DESCRIPTION_CHANGED = 1u << 3,
    VOGUE_ACCESSIBILITY_ROLE_CHANGED = 1u << 4,
    VOGUE_ACCESSIBILITY_STATE_CHANGED = 1u << 5,
    VOGUE_ACCESSIBILITY_ACTIONS_CHANGED = 1u << 6,
    VOGUE_ACCESSIBILITY_TREE_CHANGED = 1u << 7,
    VOGUE_ACCESSIBILITY_BOUNDS_CHANGED = 1u << 8,
    VOGUE_ACCESSIBILITY_FOCUS_CHANGED = 1u << 9,
    VOGUE_ACCESSIBILITY_RELATIONS_CHANGED = 1u << 10
};

/* Initialize the entire node, including a nonempty accessible name, before use.
 * Refreshes ID, role, state (preserving host-owned OFFSCREEN), and actions.
 * Buttons are leaves: child links and controls are cleared. Other host fields
 * are preserved.
 * The transient held-down appearance is not a toggle/checked semantic state.
 */
VOGUE_API void vogue_button_accessibility(
    const VogueContext *context, const VogueButton *button,
    VogueAccessibilityNode *node);

/* Route exactly one action to a live, exposed button on the context's thread.
 * Marshal native accessibility callbacks to that thread in the host.
 * INVOKE works without focus and cancels this button's pending gesture to avoid
 * a second activation on release. Disabled buttons reject both actions.
 */
VOGUE_API VogueEffects vogue_button_accessibility_action(
    VogueContext *context, const VogueButton *button, uint32_t action);

/* Adds popup/expanded state and the controls relation to button semantics. */
VOGUE_API void vogue_menu_trigger_accessibility(
    const VogueContext *context, const VogueButton *trigger,
    const VogueMenu *menu, VogueAccessibilityNode *node);

/* Initialize names, bounds, and host fields as for button semantics. Publish the
 * menu and items only while open. The menu helper fills child endpoints; the item
 * helper fills parent/sibling/leaf links. Other host fields are preserved, except
 * controls is cleared. Disabled items advertise FOCUS but never INVOKE.
 */
VOGUE_API void vogue_menu_accessibility(
    const VogueContext *context, const VogueMenu *menu, VogueAccessibilityNode *node);
VOGUE_API void vogue_menu_item_accessibility(
    const VogueContext *context, const VogueMenu *menu, uint32_t index,
    VogueAccessibilityNode *node);

/* NONE targets the container (FOCUS or DISMISS); an item accepts FOCUS or INVOKE.
 * Route on the context's thread. Closed menus reject all accessibility actions.
 * INVOKE honors DISABLED/KEEP_OPEN; a successful invocation cancels its pending gesture.
 */
VOGUE_API VogueMenuEffects vogue_menu_accessibility_action(
    VogueContext *context, VogueMenu *menu, uint32_t index, uint32_t action);

/* Compare snapshots when a node changes, then publish the flags via a platform
 * adapter. Null before/after means addition/removal; both null means no change.
 * Replacing an ID reports removal plus addition. Text comparison uses pointer,
 * length, and revision only. No allocation, string scan, or tree traversal.
 */
VOGUE_API uint32_t vogue_accessibility_changes(
    const VogueAccessibilityNode *before, const VogueAccessibilityNode *after);

#ifdef __cplusplus
}
#endif

#endif
