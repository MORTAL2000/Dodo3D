
#pragma once

#include <types.h>
#include <maths.h>
#include <log.h>
#include <vector>
#include <id.h>

namespace Dodo
{

struct AABB
{
  vec3 mCenter;
  vec3 mExtents;
};

enum AttributeType
{
  VERTEX_POSITION          = 0,
  VERTEX_UV                = 1,
  VERTEX_NORMAL            = 2,
  VERTEX_COLOR             = 3,
  VERTEX_ATTRIBUTE_4       = 4,
  VERTEX_ATTRIBUTE_5       = 5,
  VERTEX_ATTRIBUTE_COUNT   = 6
};

struct AttributeDescription
{
  AttributeDescription()
  :mBuffer(0)
  ,mOffset(0)
  ,mStride(0)
  ,mComponentType(TYPE_COUNT)
  ,mDivisor(0)
  {}

  AttributeDescription( Type componentType, size_t offset, size_t stride, u32 buffer = 0, u32 divisor = 0 )
  :mBuffer(buffer)
  ,mOffset(offset)
  ,mStride(stride)
  ,mComponentType(componentType)
  ,mDivisor(divisor)
  {
  }

  u32           mBuffer;
  size_t        mOffset;          //Offset of the attribute in the stream
  size_t        mStride;
  Type          mComponentType;
  u32           mDivisor;
};

struct VertexFormat
{
public:
  VertexFormat();
  void SetAttribute( AttributeType type, const AttributeDescription& attr );
  const AttributeDescription& GetAttributeDescription( u32 index ) const;
  bool IsAttributeEnabled( u32 index ) const;
  u32 AttributeCount() const;
  size_t VertexSize() const;

private:
  AttributeDescription  mAttribute[VERTEX_ATTRIBUTE_COUNT];
  bool                  mEnable[VERTEX_ATTRIBUTE_COUNT];     //@TODO:Make this a bit field
  size_t                mVertexSize;                        //Size in bytes
};

struct Mesh
{
public:
  Mesh();
  ~Mesh();


  AABB          mAABB;

  u32           mVertexBuffer;
  u32           mIndexBuffer;
  size_t        mVertexCount;
  size_t        mIndexCount;
  VertexFormat  mVertexFormat;
  u32           mPrimitive;
};

}
