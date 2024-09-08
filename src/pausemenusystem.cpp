#include "pausemenusystem.h"
#include "fetext.h"


PauseMenuSystem::PauseMenuSystem(font_index a2)
    : FEMenuSystem(17, a2)
{
    CDECL_CALL(0x00647E50, a2);
}