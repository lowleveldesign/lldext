#pragma once

#include <stdint.h>

struct T0 { };
struct T1 { };
struct T2 { };
struct T3 { };
struct T4 { };
struct T5 { };
struct T6 { };
struct T7 { };
struct T8 { };
struct T9 { };

struct NvWchar { wchar_t c; };

struct NvGuid {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
};

