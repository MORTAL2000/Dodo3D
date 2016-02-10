

#pragma once

#include <maths.h>
#include <types.h>


namespace Dodo
{
  struct Material
  {
    vec3 mDiffuseColor;
    vec3 mSpecularColor;

    u32 mDiffuseMap;
    u32 mSpecularMap;
    u32 mNormalMap;
  };
}
