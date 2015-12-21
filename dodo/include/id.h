
#pragma once

#include <types.h>

namespace Dodo
{

struct Id
{
  Id():mIndex(-1),mGeneration(-1){}
  Id( unsigned int index, unsigned int gen ):mIndex(index),mGeneration(gen){}

  bool operator==( const Id& id )
  {
    return ( (mIndex == id.mIndex) && (mGeneration == id.mGeneration) );
  }

  bool operator!=( const Id& id )
  {
    return ( (mIndex != id.mIndex) || (mGeneration != id.mGeneration) );
  }


  u32 mIndex;
  u32 mGeneration;
};

static const Id INVALID_ID = Id();

}
