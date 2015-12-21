

#include <image.h>
#include <cstdlib>
#define STB_IMAGE_IMPLEMENTATION
#include "stb-image.h"

using namespace Dodo;

Image::Image()
:mWidth(0),
 mHeight(0),
 mDepth(0),
 mFormat(FORMAT_INVALID),
 mData(0)
{
}

Image::Image( s32 width, s32 height, s32 depth, TextureFormat format, u8* data )
:mWidth(width),
 mHeight(height),
 mDepth(depth),
 mFormat(format),
 mData(0)
{
  if( data )
  {
    size_t size( TextureFomatSize(format)* width * height );
    mData = (u8*)malloc(size);
    memcpy( mData, data, size);
  }
}

Image::Image( const char* path, bool flipY )
:mWidth(0),
 mHeight(0),
 mDepth(0),
 mFormat(FORMAT_INVALID),
 mData(0)
{
  LoadFromFile(path, flipY);
}

bool Image::LoadFromFile( const char* path, bool flipY )
{
  if( mData )
  {
    delete mData;
    mData = 0;
  }

  s32 channelCount;
  stbi_set_flip_vertically_on_load( flipY ); // flip the image vertically, so the first pixel in the output array is the bottom left
  mData = stbi_load(path, &mWidth, &mHeight, &channelCount, 0);

  mFormat = FORMAT_INVALID;
  if( mData )
  {
    switch( channelCount )
    {
      case 1:
        mFormat = FORMAT_R8;
        break;
      case 2:
        //mFormat = FORMAT_RG;
        break;
      case 3:
        mFormat = FORMAT_RGB8;
        break;
      case 4:
        mFormat = FORMAT_RGBA8;
        break;
      default:
        break;
    }

    return true;
  }
  else
  {
    DODO_LOG("Failed to load image %s", path );
    return false;
  }
}

Image::~Image()
{
  if( mData )
    free(mData);
}
