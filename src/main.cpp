#include <napi.h>
#include "Focus/focus.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) 
{
    return focus::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)