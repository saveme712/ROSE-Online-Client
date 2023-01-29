#include "rose.h"
#include <stdio.h>

void Entity::get_name(char* name, size_t size)
{
    sprintf_s(name, size, "unknown");

    if (get_entity_type() == et_avatar)
    {
        sprintf_s(name, size, "avatar");
    }
    else if (get_entity_type() == et_cart)
    {
        sprintf_s(name, size, "cart");
    }
    else if (get_entity_type() == et_cgear)
    {
        sprintf_s(name, size, "cgear");
    }
    else if (get_entity_type() == et_cnst)
    {
        sprintf_s(name, size, "cnst");
    }
    else if (get_entity_type() == et_collision)
    {
        sprintf_s(name, size, "collision");
    }
    else if (get_entity_type() == et_eventobject)
    {
        sprintf_s(name, size, "eventobject");
    }
    else if (get_entity_type() == et_ground)
    {
        sprintf_s(name, size, "ground");
    }
    else if (get_entity_type() == et_item)
    {
        sprintf_s(name, size, "item");
    }
    else if (get_entity_type() == et_mob)
    {
        sprintf_s(name, size, "mob");
    }
    else if (get_entity_type() == et_morph)
    {
        sprintf_s(name, size, "morph");
    }
    else if (get_entity_type() == et_npc)
    {
        sprintf_s(name, size, "npc");
    }
    else if (get_entity_type() == et_null)
    {
        sprintf_s(name, size, "null");
    }
    else if (get_entity_type() == et_user)
    {
        sprintf_s(name, size, "user");
    }
}