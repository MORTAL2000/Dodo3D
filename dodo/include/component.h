
#pragma once

#include <node.h>

namespace Dodo
{

struct Component
{	
  enum Type
  {
    TRANSFORM,
    RENDERER,
    COUNT
  };

  Component( Component::Type t )
  :mType(t)
  {}



  Type  mType;
};

}
