#include "internal.h"

static uint32_t vogue_menu_owns_focus(const VogueContext *context, const VogueMenu *menu)
{
    return context->_focused == menu->_id ||
        vogue_menu_focused(context, menu) != VOGUE_MENU_ITEM_NONE;
}

static void vogue_menu_hover(VogueContext *context, VogueMenu *menu, uint32_t index)
{
    if (menu->_hovered != VOGUE_MENU_ITEM_NONE &&
        context->_hovered == vogue_internal_menu_item(menu, menu->_hovered)->id) {
        context->_hovered = VOGUE_ID_NONE;
    }
    menu->_hovered = index;
    if (index != VOGUE_MENU_ITEM_NONE) {
        context->_hovered = vogue_internal_menu_item(menu, index)->id;
    }
}

static VogueEffects vogue_menu_move_focus(
    VogueContext *context, VogueMenu *menu, uint32_t index, uint32_t keep_capture)
{
    VogueEffects effects = {0};
    VogueId id = index == VOGUE_MENU_ITEM_NONE
        ? menu->_id : vogue_internal_menu_item(menu, index)->id;

    if (!(keep_capture != 0 && context->_active == menu->_id &&
          context->_press == VOGUE_PRESS_POINTER) &&
        (context->_focused != id || context->_active == menu->_id)) {
        effects = vogue_internal_cancel(context);
    }
    if (context->_focused != id) {
        context->_focused = id;
        effects.flags |= VOGUE_EFFECT_FOCUS;
        effects.focus_target = id;
    }
    menu->_focused = index;
    return effects;
}

static VogueMenuEffects vogue_menu_activate(
    VogueContext *context, VogueMenu *menu, uint32_t index)
{
    VogueMenuEffects result = {0};

    if (index != VOGUE_MENU_ITEM_NONE &&
        vogue_internal_menu_item(menu, index)->disabled == 0) {
        result = vogue_menu_close(context, menu, 1);
        result.effects.flags |= VOGUE_EFFECT_ACTIVATE;
        result.activated_index = index;
    }
    return result;
}

void vogue_menu_init(VogueMenu *menu, VogueId id, const VogueMenuItem *items, uint32_t count)
{
    assert(menu != 0);
    assert(id != VOGUE_ID_NONE);
    assert(items != 0 || count == 0);
    *menu = (VogueMenu){0};
    menu->_id = id;
    menu->_items = items;
    menu->_count = count;
    menu->_focused = VOGUE_MENU_ITEM_NONE;
    menu->_hovered = VOGUE_MENU_ITEM_NONE;
}

VogueMenuEffects vogue_menu_open(
    VogueContext *context, VogueMenu *menu, uint32_t initial_index)
{
    VogueMenuEffects result = {0};
    VogueId target;

    assert(context != 0);
    assert(menu != 0);
    assert(menu->_id != VOGUE_ID_NONE);
    if (menu->_open != 0) {
        return result;
    }
    if (menu->_count == 0) {
        initial_index = VOGUE_MENU_ITEM_NONE;
    }
    target = initial_index == VOGUE_MENU_ITEM_NONE
        ? menu->_id : vogue_internal_menu_item(menu, initial_index)->id;
    menu->_return_focus = context->_focused;
    result.effects = vogue_internal_cancel(context);
    context->_focused = target;
    context->_hovered = VOGUE_ID_NONE;
    menu->_focused = initial_index;
    menu->_hovered = VOGUE_MENU_ITEM_NONE;
    menu->_open = 1;
    result.effects.flags |= VOGUE_EFFECT_FOCUS | VOGUE_EFFECT_SEMANTICS_CHANGED;
    result.effects.focus_target = target;
    result.flags = VOGUE_MENU_EFFECT_OPENED;
    return result;
}

VogueMenuEffects vogue_menu_close(
    VogueContext *context, VogueMenu *menu, uint32_t restore_focus)
{
    VogueMenuEffects result = {0};

    assert(context != 0);
    assert(menu != 0);
    assert(menu->_id != VOGUE_ID_NONE);
    if (menu->_open == 0) {
        return result;
    }
    if (context->_active == menu->_id) {
        result.effects = vogue_internal_cancel(context);
    }
    if (vogue_menu_owns_focus(context, menu) != 0) {
        context->_focused = restore_focus != 0 ? menu->_return_focus : VOGUE_ID_NONE;
        result.effects.flags |= VOGUE_EFFECT_FOCUS;
        result.effects.focus_target = context->_focused;
    }
    vogue_menu_hover(context, menu, VOGUE_MENU_ITEM_NONE);
    menu->_focused = VOGUE_MENU_ITEM_NONE;
    menu->_open = 0;
    result.effects.flags |= VOGUE_EFFECT_SEMANTICS_CHANGED;
    result.flags = VOGUE_MENU_EFFECT_CLOSED;
    return result;
}

uint32_t vogue_menu_is_open(const VogueMenu *menu)
{
    assert(menu != 0);
    return menu->_open;
}

uint32_t vogue_menu_focused(const VogueContext *context, const VogueMenu *menu)
{
    assert(context != 0);
    assert(menu != 0);
    if (menu->_open != 0 && menu->_focused != VOGUE_MENU_ITEM_NONE &&
        context->_focused == vogue_internal_menu_item(menu, menu->_focused)->id) {
        return menu->_focused;
    }
    return VOGUE_MENU_ITEM_NONE;
}

VogueMenuEffects vogue_menu_focus(VogueContext *context, VogueMenu *menu, uint32_t index)
{
    VogueMenuEffects result = {0};

    assert(context != 0);
    assert(menu != 0);
    if (menu->_open != 0) {
        result.effects = vogue_menu_move_focus(context, menu, index, 0);
    }
    return result;
}

static VogueMenuEffects vogue_menu_key(
    VogueContext *context, VogueMenu *menu, const VogueEvent *event)
{
    VogueMenuEffects result = {0};
    uint32_t index = vogue_menu_focused(context, menu);

    switch (event->key) {
    case VOGUE_KEY_UP:
    case VOGUE_KEY_DOWN:
    case VOGUE_KEY_HOME:
    case VOGUE_KEY_END:
        if (event->type == VOGUE_EVENT_KEY_DOWN && menu->_count != 0) {
            if (event->key == VOGUE_KEY_HOME) {
                index = 0;
            } else if (event->key == VOGUE_KEY_END) {
                index = menu->_count - 1;
            } else if (event->key == VOGUE_KEY_UP) {
                index = index == VOGUE_MENU_ITEM_NONE || index == 0
                    ? menu->_count - 1 : index - 1;
            } else {
                index = index == VOGUE_MENU_ITEM_NONE || index + 1 == menu->_count
                    ? 0 : index + 1;
            }
            result.effects = vogue_menu_move_focus(context, menu, index, 0);
        }
        break;
    case VOGUE_KEY_ENTER:
    case VOGUE_KEY_SPACE:
        if (event->type == VOGUE_EVENT_KEY_DOWN &&
            (event->flags & VOGUE_EVENT_REPEAT) == 0) {
            result = vogue_menu_activate(context, menu, index);
        }
        break;
    case VOGUE_KEY_ESCAPE:
    case VOGUE_KEY_TAB:
        if (event->type == VOGUE_EVENT_KEY_DOWN &&
            (event->flags & VOGUE_EVENT_REPEAT) == 0) {
            result = vogue_menu_close(context, menu, 1);
            if (event->key == VOGUE_KEY_TAB) {
                result.flags |= (event->flags & VOGUE_EVENT_SHIFT) != 0
                    ? VOGUE_MENU_EFFECT_FOCUS_PREVIOUS : VOGUE_MENU_EFFECT_FOCUS_NEXT;
            }
        }
        break;
    default:
        return result;
    }
    result.flags |= VOGUE_MENU_EFFECT_HANDLED;
    return result;
}

VogueMenuEffects vogue_menu_handle(
    VogueContext *context, VogueMenu *menu, uint32_t index, const VogueEvent *event)
{
    VogueMenuEffects result = {0};
    const VogueMenuItem *item;
    uint32_t captured;

    assert(context != 0);
    assert(menu != 0);
    assert(event != 0);
    if (menu->_open == 0) {
        return result;
    }
    if (vogue_menu_owns_focus(context, menu) == 0) {
        return vogue_menu_close(context, menu, 0);
    }
    if (event->type == VOGUE_EVENT_KEY_DOWN || event->type == VOGUE_EVENT_KEY_UP) {
        return vogue_menu_key(context, menu, event);
    }
    if (event->type < VOGUE_EVENT_POINTER_MOVE || event->type > VOGUE_EVENT_POINTER_CANCEL) {
        return result;
    }
    captured = context->_press == VOGUE_PRESS_POINTER;
    if (captured != 0 &&
        (context->_active != menu->_id || context->_pointer != event->pointer_id)) {
        return result;
    }
    if (event->type == VOGUE_EVENT_POINTER_CANCEL) {
        if (captured != 0) {
            result.effects = vogue_internal_cancel(context);
            vogue_menu_hover(context, menu, VOGUE_MENU_ITEM_NONE);
            result.flags = VOGUE_MENU_EFFECT_HANDLED;
        }
        return result;
    }
    if ((event->flags & VOGUE_EVENT_INSIDE) == 0) {
        index = VOGUE_MENU_ITEM_NONE;
    }
    item = index == VOGUE_MENU_ITEM_NONE ? 0 : vogue_internal_menu_item(menu, index);

    switch (event->type) {
    case VOGUE_EVENT_POINTER_MOVE:
        vogue_menu_hover(context, menu, index);
        if (item != 0) {
            result.effects = vogue_menu_move_focus(context, menu, index, 1);
        }
        if (captured != 0) {
            context->_inside = item != 0;
        }
        if (captured != 0 || (event->flags & VOGUE_EVENT_INSIDE) != 0) {
            result.flags = VOGUE_MENU_EFFECT_HANDLED;
        }
        break;
    case VOGUE_EVENT_POINTER_DOWN:
        if (context->_press != VOGUE_PRESS_NONE) {
            return result;
        }
        if ((event->flags & VOGUE_EVENT_INSIDE) == 0) {
            return vogue_menu_close(context, menu, 0);
        }
        result.flags = VOGUE_MENU_EFFECT_HANDLED;
        if (event->button != VOGUE_POINTER_PRIMARY) {
            return result;
        }
        vogue_menu_hover(context, menu, index);
        if (item != 0) {
            result.effects = vogue_menu_move_focus(context, menu, index, 0);
            if (item->disabled == 0) {
                context->_active = menu->_id;
                context->_pointer = event->pointer_id;
                context->_press = VOGUE_PRESS_POINTER;
                context->_inside = 1;
                result.effects.flags |= VOGUE_EFFECT_CAPTURE_POINTER;
                result.effects.pointer_id = event->pointer_id;
                result.effects.pointer_target = menu->_id;
            }
        }
        break;
    case VOGUE_EVENT_POINTER_UP:
        if (captured == 0 || event->button != VOGUE_POINTER_PRIMARY) {
            return result;
        }
        if (item != 0 && item->disabled == 0) {
            result = vogue_menu_activate(context, menu, index);
        } else {
            vogue_menu_hover(context, menu, index);
            result.effects = item != 0
                ? vogue_menu_move_focus(context, menu, index, 0)
                : vogue_internal_cancel(context);
        }
        result.flags |= VOGUE_MENU_EFFECT_HANDLED;
        break;
    default:
        break;
    }
    return result;
}

uint32_t vogue_menu_item_state(
    const VogueContext *context, const VogueMenu *menu, uint32_t index)
{
    const VogueMenuItem *item = vogue_internal_menu_item(menu, index);
    uint32_t state = item->disabled != 0 ? VOGUE_STATE_DISABLED : 0;

    assert(context != 0);
    if (menu->_open == 0) {
        return state;
    }
    if (context->_focused == item->id) {
        state |= VOGUE_STATE_FOCUSED;
    }
    if (context->_hovered == item->id) {
        state |= VOGUE_STATE_HOVERED;
        if (item->disabled == 0 && context->_active == menu->_id &&
            context->_press == VOGUE_PRESS_POINTER && context->_inside != 0) {
            state |= VOGUE_STATE_PRESSED;
        }
    }
    return state;
}

VogueMenuEffects vogue_menu_accessibility_action(
    VogueContext *context, VogueMenu *menu, uint32_t index, uint32_t action)
{
    VogueMenuEffects result = {0};

    assert(context != 0);
    assert(menu != 0);
    if (menu->_open != 0) {
        if (action == VOGUE_ACTION_FOCUS) {
            result = vogue_menu_focus(context, menu, index);
        } else if (action == VOGUE_ACTION_DISMISS && index == VOGUE_MENU_ITEM_NONE) {
            result = vogue_menu_close(context, menu, 1);
        } else if (action == VOGUE_ACTION_INVOKE && index != VOGUE_MENU_ITEM_NONE) {
            result = vogue_menu_activate(context, menu, index);
        }
    }
    return result;
}
