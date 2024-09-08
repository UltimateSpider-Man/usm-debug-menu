#include "marker.h"

#include "common.h"

VALIDATE_SIZE(marker, 0x68);

marker::marker(const string_hash &a2, uint32_t a3) : entity_base(a2, a3, false) {
    this->field_64 = 0;
    this->field_60 = 0;
    this->field_5C = 0;
    this->field_48 = 0;
    this->field_4C = 0;
    this->field_50 = 0;
    this->field_58 = 0;
    this->field_54 = 0;
}
