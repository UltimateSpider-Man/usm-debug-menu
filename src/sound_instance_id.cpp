#include "sound_instance_id.h"

#include "common.h"

#include "variable.h"

#include "func_wrapper.h"

VALIDATE_SIZE(sound_instance_slot, 0x54);


sound_instance_slot*& s_sound_instance_slots = var<sound_instance_slot*>(0x009682E0);

void sound_instance::stop() {
    CDECL_CALL(0x0053E6F0, this);
}

sound_instance_id sub_60B960(string_hash a2, Float a3, Float a4) {
    sound_instance_id result;
    CDECL_CALL(0x0060B960, &result, a2, a3, a4);

    return result;
}

sound_instance *sound_instance_id::get_sound_instance_ptr() {
    return (sound_instance *) CDECL_CALL(0x0050EF00, this);
}
