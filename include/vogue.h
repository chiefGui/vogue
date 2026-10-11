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
    VOGUE_EFFECT_RELEASE_POINTER = 1u << 3
};

/* Apply returned effects before dispatching another event. Release an old
 * capture before moving focus. ACTIVATE belongs to the button being handled.
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

/* All pointer arguments must be non-null and refer to initialized objects,
 * except that the init functions accept uninitialized output storage.
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

/* Deliver an event directly to its target; the core performs no hit testing.
 * While captured, send pointer events to the captured button, even outside.
 * Send keyboard events to the focused button. See README.md for routing rules.
 */
VOGUE_API VogueEffects vogue_button_handle(
    VogueContext *context, const VogueButton *button, const VogueEvent *event);

/* Read current state without mutation. This performs no allocation or lookup. */
VOGUE_API uint32_t vogue_button_state(
    const VogueContext *context, const VogueButton *button);

#ifdef __cplusplus
}
#endif

#endif
