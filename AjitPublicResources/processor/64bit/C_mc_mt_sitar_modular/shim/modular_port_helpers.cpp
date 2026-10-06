#include "modular_port_helpers.h"

extern "C" {
bool pullbool(void* obj, bool* value, bool sync);
bool pushbool(void* obj, bool value, bool sync);
bool pullchar(void* obj, uint8_t* value, bool sync);
bool pushchar(void* obj, uint8_t value, bool sync);
bool pullword(void* obj, uint32_t* value, bool sync);
bool pushword(void* obj, uint32_t value, bool sync);
bool pulldword(void* obj, uint64_t* value, bool sync);
bool pushdword(void* obj, uint64_t value, bool sync);
}

extern "C" bool mod_pull_bool(void* port, bool* value)
{
  return pullbool(port, value, false);
}

extern "C" bool mod_push_bool(void* port, bool value)
{
  return pushbool(port, value, false);
}

extern "C" bool mod_pull_u8(void* port, uint8_t* value)
{
  return pullchar(port, value, false);
}

extern "C" bool mod_push_u8(void* port, uint8_t value)
{
  return pushchar(port, value, false);
}

extern "C" bool mod_pull_u32(void* port, uint32_t* value)
{
  return pullword(port, value, false);
}

extern "C" bool mod_push_u32(void* port, uint32_t value)
{
  return pushword(port, value, false);
}

extern "C" bool mod_pull_u64(void* port, uint64_t* value)
{
  return pulldword(port, value, false);
}

extern "C" bool mod_push_u64(void* port, uint64_t value)
{
  return pushdword(port, value, false);
}
