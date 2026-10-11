#include "vogue_accessibility.h"
#include "internal.h"

void vogue_button_accessibility(
    const VogueContext *context, const VogueButton *button,
    VogueAccessibilityNode *node)
{
    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    assert(node != 0);
    assert(node->name.data != 0 && node->name.length != 0);
    assert(node->description.data != 0 || node->description.length == 0);
    assert(node->bounds.width >= 0.0f && node->bounds.height >= 0.0f);

    node->id = button->_id;
    node->role = VOGUE_ROLE_BUTTON;
    node->first_child = VOGUE_ID_NONE;
    node->last_child = VOGUE_ID_NONE;
    node->state &= VOGUE_ACCESSIBLE_OFFSCREEN;
    node->actions = 0;
    if (button->_disabled != 0) {
        node->state |= VOGUE_ACCESSIBLE_DISABLED;
    } else {
        node->state |= VOGUE_ACCESSIBLE_FOCUSABLE;
        if (context->_focused == button->_id) {
            node->state |= VOGUE_ACCESSIBLE_FOCUSED;
        }
        node->actions = VOGUE_ACTION_FOCUS | VOGUE_ACTION_INVOKE;
    }
}

VogueEffects vogue_button_accessibility_action(
    VogueContext *context, const VogueButton *button, uint32_t action)
{
    VogueEffects effects = {0};

    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    if (button->_disabled != 0) {
        return effects;
    }
    if (action == VOGUE_ACTION_FOCUS) {
        return vogue_button_focus(context, button);
    }
    if (action == VOGUE_ACTION_INVOKE) {
        if (context->_active == button->_id) {
            effects = vogue_internal_cancel(context);
        }
        effects.flags |= VOGUE_EFFECT_ACTIVATE;
    }
    return effects;
}

static uint32_t vogue_text_changed(VogueAccessibleText before, VogueAccessibleText after)
{
    return before.data != after.data || before.length != after.length ||
        before.revision != after.revision;
}

uint32_t vogue_accessibility_changes(
    const VogueAccessibilityNode *before, const VogueAccessibilityNode *after)
{
    uint32_t changes = 0;

    if (before == 0) {
        return after != 0 ? VOGUE_ACCESSIBILITY_ADDED : 0;
    }
    if (after == 0) {
        return VOGUE_ACCESSIBILITY_REMOVED;
    }
    if (before->id != after->id) {
        return VOGUE_ACCESSIBILITY_REMOVED | VOGUE_ACCESSIBILITY_ADDED;
    }
    if (vogue_text_changed(before->name, after->name) != 0) {
        changes |= VOGUE_ACCESSIBILITY_NAME_CHANGED;
    }
    if (vogue_text_changed(before->description, after->description) != 0) {
        changes |= VOGUE_ACCESSIBILITY_DESCRIPTION_CHANGED;
    }
    if (before->role != after->role) {
        changes |= VOGUE_ACCESSIBILITY_ROLE_CHANGED;
    }
    if (before->state != after->state) {
        changes |= VOGUE_ACCESSIBILITY_STATE_CHANGED;
        if (((before->state ^ after->state) & VOGUE_ACCESSIBLE_FOCUSED) != 0) {
            changes |= VOGUE_ACCESSIBILITY_FOCUS_CHANGED;
        }
    }
    if (before->actions != after->actions) {
        changes |= VOGUE_ACCESSIBILITY_ACTIONS_CHANGED;
    }
    if (before->parent != after->parent || before->first_child != after->first_child ||
        before->last_child != after->last_child ||
        before->previous_sibling != after->previous_sibling ||
        before->next_sibling != after->next_sibling) {
        changes |= VOGUE_ACCESSIBILITY_TREE_CHANGED;
    }
    if (before->bounds.x != after->bounds.x || before->bounds.y != after->bounds.y ||
        before->bounds.width != after->bounds.width ||
        before->bounds.height != after->bounds.height) {
        changes |= VOGUE_ACCESSIBILITY_BOUNDS_CHANGED;
    }
    return changes;
}
