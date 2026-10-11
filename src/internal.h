#ifndef VOGUE_INTERNAL_H
#define VOGUE_INTERNAL_H

#include "vogue.h"
#include <assert.h>

enum {
    VOGUE_PRESS_NONE = 0,
    VOGUE_PRESS_POINTER,
    VOGUE_PRESS_ENTER,
    VOGUE_PRESS_SPACE
};

static inline VogueEffects vogue_internal_cancel(VogueContext *context)
{
    VogueEffects effects = {0};

    if (context->_press == VOGUE_PRESS_POINTER) {
        effects.flags = VOGUE_EFFECT_RELEASE_POINTER;
        effects.pointer_id = context->_pointer;
        effects.pointer_target = context->_active;
    }

    context->_active = VOGUE_ID_NONE;
    context->_pointer = 0;
    context->_press = VOGUE_PRESS_NONE;
    context->_inside = 0;
    return effects;
}

static inline VogueEffects vogue_internal_focus(VogueContext *context, VogueId id)
{
    VogueEffects effects = {0};

    if (context->_focused != id) {
        effects = vogue_internal_cancel(context);
        context->_focused = id;
        effects.flags |= VOGUE_EFFECT_FOCUS;
        effects.focus_target = id;
    }

    return effects;
}

static inline void vogue_internal_hover(
    VogueContext *context, VogueId id, uint32_t flags)
{
    if ((flags & VOGUE_EVENT_INSIDE) != 0) {
        context->_hovered = id;
    } else if (context->_hovered == id) {
        context->_hovered = VOGUE_ID_NONE;
    }
}

#endif
