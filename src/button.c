#include "internal.h"

void vogue_button_init(VogueButton *button, VogueId id)
{
    assert(button != 0);
    assert(id != VOGUE_ID_NONE);
    button->_id = id;
    button->_disabled = 0;
}

VogueEffects vogue_button_set_disabled(
    VogueContext *context, VogueButton *button, uint32_t disabled)
{
    VogueEffects effects = {0};

    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    button->_disabled = disabled != 0;
    if (button->_disabled != 0) {
        effects = vogue_context_forget(context, button->_id);
    }
    return effects;
}

VogueEffects vogue_button_focus(VogueContext *context, const VogueButton *button)
{
    VogueEffects effects = {0};

    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    if (button->_disabled == 0) {
        effects = vogue_internal_focus(context, button->_id);
    }
    return effects;
}

VogueEffects vogue_button_handle(
    VogueContext *context, const VogueButton *button, const VogueEvent *event)
{
    VogueEffects effects = {0};
    VogueId id;

    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    assert(event != 0);
    id = button->_id;
    if (button->_disabled != 0) {
        return effects;
    }

    switch (event->type) {
    case VOGUE_EVENT_POINTER_MOVE:
        if (context->_press == VOGUE_PRESS_POINTER) {
            if (context->_active != id || context->_pointer != event->pointer_id) {
                return effects;
            }
            context->_inside = (event->flags & VOGUE_EVENT_INSIDE) != 0;
        }
        vogue_internal_hover(context, id, event->flags);
        break;

    case VOGUE_EVENT_POINTER_DOWN:
        if (event->button != VOGUE_POINTER_PRIMARY ||
            (event->flags & VOGUE_EVENT_INSIDE) == 0 ||
            context->_press != VOGUE_PRESS_NONE) {
            return effects;
        }
        effects = vogue_internal_focus(context, id);
        context->_hovered = id;
        context->_active = id;
        context->_pointer = event->pointer_id;
        context->_press = VOGUE_PRESS_POINTER;
        context->_inside = 1;
        effects.flags |= VOGUE_EFFECT_CAPTURE_POINTER;
        effects.pointer_id = event->pointer_id;
        effects.pointer_target = id;
        break;

    case VOGUE_EVENT_POINTER_UP:
        if (context->_press != VOGUE_PRESS_POINTER || context->_active != id ||
            context->_pointer != event->pointer_id ||
            event->button != VOGUE_POINTER_PRIMARY) {
            return effects;
        }
        vogue_internal_hover(context, id, event->flags);
        effects = vogue_internal_cancel(context);
        if ((event->flags & VOGUE_EVENT_INSIDE) != 0) {
            effects.flags |= VOGUE_EFFECT_ACTIVATE;
        }
        break;

    case VOGUE_EVENT_POINTER_CANCEL:
        if (context->_press == VOGUE_PRESS_POINTER && context->_active == id &&
            context->_pointer == event->pointer_id) {
            effects = vogue_internal_cancel(context);
            if (context->_hovered == id) {
                context->_hovered = VOGUE_ID_NONE;
            }
        }
        break;

    case VOGUE_EVENT_KEY_DOWN:
        if (context->_focused != id || context->_press != VOGUE_PRESS_NONE ||
            (event->flags & VOGUE_EVENT_REPEAT) != 0) {
            return effects;
        }
        if (event->key == VOGUE_KEY_ENTER) {
            context->_active = id;
            context->_press = VOGUE_PRESS_ENTER;
            effects.flags = VOGUE_EFFECT_ACTIVATE;
        } else if (event->key == VOGUE_KEY_SPACE) {
            context->_active = id;
            context->_press = VOGUE_PRESS_SPACE;
        }
        break;

    case VOGUE_EVENT_KEY_UP:
        if (context->_focused != id || context->_active != id) {
            return effects;
        }
        if (event->key == VOGUE_KEY_SPACE && context->_press == VOGUE_PRESS_SPACE) {
            effects = vogue_internal_cancel(context);
            effects.flags |= VOGUE_EFFECT_ACTIVATE;
        } else if (event->key == VOGUE_KEY_ENTER &&
                   context->_press == VOGUE_PRESS_ENTER) {
            effects = vogue_internal_cancel(context);
        }
        break;

    default:
        break;
    }
    return effects;
}

uint32_t vogue_button_state(const VogueContext *context, const VogueButton *button)
{
    uint32_t state = 0;

    assert(context != 0);
    assert(button != 0);
    assert(button->_id != VOGUE_ID_NONE);
    if (button->_disabled != 0) {
        return VOGUE_STATE_DISABLED;
    }
    if (context->_hovered == button->_id) {
        state |= VOGUE_STATE_HOVERED;
    }
    if (context->_focused == button->_id) {
        state |= VOGUE_STATE_FOCUSED;
    }
    if (context->_active == button->_id &&
        (context->_press != VOGUE_PRESS_POINTER || context->_inside != 0)) {
        state |= VOGUE_STATE_PRESSED;
    }
    return state;
}
