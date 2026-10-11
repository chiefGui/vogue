#ifndef VOGUE_ACCESSIBILITY_H
#define VOGUE_ACCESSIBILITY_H

#include "vogue.h"

#ifdef __cplusplus
extern "C" {
#endif

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
