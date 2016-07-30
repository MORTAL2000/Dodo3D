
#include <mesh.h>
#include <gl-renderer.h>

using namespace Dodo;

VertexFormat::VertexFormat()
:mVertexSize(0)
{
  for( u32 i(0); i<VERTEX_ATTRIBUTE_COUNT; ++i )
  {
    mEnable[i] = false;
  }
}

void VertexFormat::SetAttribute( AttributeType index, const AttributeDescription& attr )
{
  mAttribute[index] = attr;
  mEnable[index] = true;

  mVertexSize += Dodo::TypeSize( attr.mComponentType );
}

const AttributeDescription& VertexFormat::GetAttributeDescription( u32 index ) const
{
  return mAttribute[index];
}

bool VertexFormat::IsAttributeEnabled( u32 index ) const
{
  return mEnable[index];
}

u32 VertexFormat::AttributeCount() const
{
  return VERTEX_ATTRIBUTE_COUNT;
}

size_t VertexFormat::VertexSize() const
{
  return mVertexSize;
}


Mesh::Mesh()
:mVertexBuffer(0),
 mIndexBuffer(0),
 mVertexCount(0),
 mIndexCount(0),
 mPrimitive(PRIMITIVE_TRIANGLES)
{}

Mesh::~Mesh()
{
}
