#pragma once

#include <log.h>
#include <types.h>

namespace Dodo
{

enum TextureFormat
{
  FORMAT_INVALID = 0,
  FORMAT_R8,
  FORMAT_RGB8,
  FORMAT_RGBA8,
  FORMAT_RGB16F,
  FORMAT_RGBA16F,
  FORMAT_RGB32F,
  FORMAT_RGBA32F,
  FORMAT_GL_DEPTH,
  FORMAT_GL_DEPTH_STENCIL
};

inline u8 TextureFomatSize( TextureFormat t )
{
  switch( t )
  {
    case FORMAT_R8:
      return 1;
    case FORMAT_RGB8:
      return 3;
    case FORMAT_RGBA8:
      return 4;
    case FORMAT_RGB16F:
      return 6;
    case FORMAT_RGBA16F:
      return 8;
    case FORMAT_RGB32F:
      return 12;
    case FORMAT_RGBA32F:
      return 16;
    case FORMAT_GL_DEPTH:
      return 24;
    case FORMAT_GL_DEPTH_STENCIL:
      return 32;
    default:
      return 0;
  }

}

struct Image
{
  Image();
  Image( s32 width, s32 height, s32 depth, TextureFormat format, u8* data = 0);
  Image( const char* path,  bool flipY = true );

  ~Image();

  bool LoadFromFile( const char* path, bool flipY );

  s32             mWidth;
  s32             mHeight;
  s32             mDepth;
  TextureFormat   mFormat;
  u8*             mData;
};

}
