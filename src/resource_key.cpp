#include "resource_key.h"


#include <cassert>
#include <cstring>
#include <string>

resource_key* resource_key::resource_key_set(resource_key& key, const string_hash& hash, int value)
{
    this->field_0 = hash;
    this->field_4 = value;
}

void resource_key::resource_key_set(const resource_key& a2)
{
    *this = a2;
}