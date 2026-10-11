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
    VOGUE_KEY_SPACE
};

enum {
    VOGUE_POINTER_PRIMARY = 0,
    VOGUE_POINTER_SECONDARY,
    VOGUE_POINTER_MIDDLE
};

enum {
    /* The pointer is inside the target's current interactive region. */
    VOGUE_EVENT_INSIDE = 1u << 0,
    VOGUE_EVENT_REPEAT = 1u << 1
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
    /* Re-query the button's semantics. Focus changes use EFFECT_FOCUS. */
    VOGUE_EFFECT_SEMANTICS_CHANGED = 1u << 4
};

/* Apply effects before the next event: release capture, move focus, acquire
 * capture, then activate. ACTIVATE belongs to the button being handled.
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

enum {
    VOGUE_ROLE_GROUP = 1,
    VOGUE_ROLE_BUTTON
};

enum {
    VOGUE_ACCESSIBLE_FOCUSABLE = 1u << 0,
    VOGUE_ACCESSIBLE_FOCUSED = 1u << 1,
    VOGUE_ACCESSIBLE_DISABLED = 1u << 2,
    VOGUE_ACCESSIBLE_OFFSCREEN = 1u << 3
};

enum {
    VOGUE_ACTION_FOCUS = 1u << 0,
    VOGUE_ACTION_INVOKE = 1u << 1
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
    VOGUE_ACCESSIBILITY_FOCUS_CHANGED = 1u << 9
};

/* Initialize the entire node, including a nonempty accessible name, before use.
 * Refreshes ID, role, state (preserving host-owned OFFSCREEN), and actions.
 * Buttons are leaves: child links are cleared. Other host fields are preserved.
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
