#include "internal.h"

void vogue_context_init(VogueContext *context)
{
    assert(context != 0);
    *context = (VogueContext){0};
}

VogueEffects vogue_context_reset(VogueContext *context)
{
    VogueEffects effects;

    assert(context != 0);
    effects = vogue_internal_cancel(context);
    if (context->_focused != VOGUE_ID_NONE) {
        effects.flags |= VOGUE_EFFECT_FOCUS;
    }
    context->_focused = VOGUE_ID_NONE;
    context->_hovered = VOGUE_ID_NONE;
    return effects;
}

VogueEffects vogue_context_forget(VogueContext *context, VogueId id)
{
    VogueEffects effects = {0};

    assert(context != 0);
    assert(id != VOGUE_ID_NONE);
    if (context->_active == id) {
        effects = vogue_internal_cancel(context);
    }
    if (context->_focused == id) {
        context->_focused = VOGUE_ID_NONE;
        effects.flags |= VOGUE_EFFECT_FOCUS;
    }
    if (context->_hovered == id) {
        context->_hovered = VOGUE_ID_NONE;
    }
    return effects;
}

VogueId vogue_context_focused(const VogueContext *context)
{
    assert(context != 0);
    return context->_focused;
}

VogueId vogue_context_captured(const VogueContext *context)
{
    assert(context != 0);
    return context->_press == VOGUE_PRESS_POINTER
        ? context->_active : VOGUE_ID_NONE;
}
