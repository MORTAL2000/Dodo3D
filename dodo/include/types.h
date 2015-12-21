#pragma once

#include <stdint.h>
#include <cstdio>
#include <float.h>

#define Bits(Value)      (Value/8.0)
#define Kilobytes(Value) (Value*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define F32_MAX FLT_MAX

typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;

typedef int8_t        s8;
typedef int16_t       s16;
typedef int32_t       s32;

typedef float         f32;
typedef double        f64;

namespace Dodo
{
enum Type
{
  U8 = 0,
  S8,
  U16,
  S16,
  U32,
  S32,
  F32,
  F64,
  F32x2,
  F32x3,
  F32x4,
  F32x16,
  TYPE_COUNT
};

inline u8 TypeSize( Dodo::Type t )
{
  switch( t )
  {
    case U8:
    case S8:
      return 1;
    case U16:
    case S16:
      return 2;
    case U32:
    case S32:
    case F32:
      return 4;
    case F32x3:
      return 12;
    case F32x4:
      return 16;
    case F32x2:
    case F64:
      return 8;
    case F32x16:
      return 64;
    default:
      return 0;
  }

}

inline u8 TypeElementCount( Dodo::Type t )
{
  switch( t )
  {
    case U8:
    case S8:
    case U16:
    case S16:
    case U32:
    case S32:
    case F32:
    case F64:
      return 1;
    case F32x2:
      return 2;
    case F32x3:
      return 3;
    case F32x4:
      return 4;
    case F32x16:
      return 16;
    default:
      return 0;
  }
}

inline u8 TypeElementSize( Dodo::Type t)
{
  switch( t )
  {
    case U8:
    case S8:
      return 1;
    case U16:
    case S16:
      return 2;
    case U32:
    case S32:
    case F32:
    case F32x3:
    case F32x2:
    case F32x4:
    case F32x16:
      return 4;
    case F64:
      return 8;
    default:
      return 0;
  }
}

}
