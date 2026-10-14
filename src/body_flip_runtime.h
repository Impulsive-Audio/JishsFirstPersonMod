#pragma once
#include "body_flip.h"
#include "settings.h"
#include "stick_runtime.h"
namespace jishsfirstpersonmod {
void tick_body_flip(const Config&,const ComfortSnapshot&,const CameraMovementSample&,double now,bool interrupted);
body_flip::Status body_flip_status();
void reset_body_flip();
}
