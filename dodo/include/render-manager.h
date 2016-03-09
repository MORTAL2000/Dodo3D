
#pragma once

#include <types.h>
#include <component-list.h>
#include <vector>
#include <maths.h>
#include <mesh.h>
#include <image.h>
#include <material.h>

namespace Dodo
{

typedef u32 BufferId;
typedef u32 TextureId;
typedef u32 VAOId;
typedef u32 ProgramId;
typedef u32 FBOId;
typedef Id MeshId;
typedef Id MultiMeshId;


enum Primitive
{
  PRIMITIVE_POINTS,
  PRIMITIVE_LINES,
  PRIMITIVE_TRIANGLES,
  PRIMITIVE_COUNT
};

enum CullFace
{
  CULL_FRONT = 0,
  CULL_BACK,
  CULL_FRONT_BACK,
  CULL_NONE
};

enum DepthTestFunction
{
  DEPTH_TEST_NEVER,
  DEPTH_TEST_ALWAYS,
  DEPTH_TEST_LESS_EQUAL,
  DEPTH_TEST_LESS,
  DEPTH_TEST_GREATER_EQUAL,
  DEPTH_TEST_GREATER,
  DEPTH_TEST_EQUAL,
  DEPTH_TEST_NOT_EQUAL,
  DEPTH_TEST_DISABLED,
};

enum Buffers
{
  COLOR_BUFFER =    1<<0,
  DEPTH_BUFFER =    1<<1,
  STENCIL_BUFFER =  1<<2
};

enum BufferMapMode
{
  BUFFER_MAP_READ = 0,
  BUFFER_MAP_WRITE = 1,
  BUFFER_MAP_READ_WRITE = 2
};

//Specifies the blending equation.
//Source is the new pixel color and destination is the color already in the frame buffer. Result is the value which will be written to the frame buffer)
enum BlendingMode
{
  BLEND_DISABLED = 0,
  BLEND_ADD = 1,                 // Result = Source * SourceFactor + Destination * DestinationFactor
  BLEND_SUBTRACT = 2,            // Result = Source * SourceFactor - Destination * DestinationFactor
  BLEND_REVERSE_SUBTRACT = 3,    // Result = Destination * DestinationFactor - Source * SourceFactor
  BLEND_MIN = 4,                 // Result = ( min(Source.r,Destionation.r),min(Source.g,Destionation.g),min(Source.b,Destionation.b),min(Source.a,Destionation.a) )
  BLEND_MAX = 5                  // Result = ( max(Source.r,Destionation.r),max(Source.g,Destionation.g),max(Source.b,Destionation.b),max(Source.a,Destionation.a) )
};

//Specifies how to compute the blending factors that will be used by the blending equation
enum BlendingFunction
{
  BLENDING_FUNCTION_ZERO,                   //(0,0,0,0)
  BLENDING_FUNCTION_ONE,                    //(1,1,1,1)
  BLENDING_SOURCE_COLOR,                    // Source
  BLENDING_ONE_MINUS_SOURCE_COLOR,          //(1,1,1,1) - Source
  BLENDING_DESTINATION_COLOR,               //Destination
  BLENDING_ONE_MINUS_DESTINATION_COLOR,     //(1,1,1,1) - Destination
  BLENDING_SOURCE_ALPHA,                    //(Source.a,Source.a,Source.a,Source.a)
  BLENDING_ONE_MINUS_SOURCE_ALPHA,             //(1 - Source.a, 1 - Source.a, 1 - Source.a, 1 - Source.a)
  BLENDING_DESTINATION_ALPHA,               //(Destination.a,Destination.a,Destination.a,Destination.a)
  BLENDING_ONE_MINUS_DESTINATION_ALPHA,     //(1 - Destination.a, 1 - Destination.a, 1 - Destination.a, 1 - Destination.a)
};

enum CubeTextureSide
{
  CUBE_POSITIVE_X = 0,
  CUBE_NEGATIVE_X = 1,
  CUBE_POSITIVE_Y = 2,
  CUBE_NEGATIVE_Y = 3,
  CUBE_POSITIVE_Z = 4,
  CUBE_NEGATIVE_Z = 5
};

struct RenderManager
{
  RenderManager();
  ~RenderManager();

  void Init();

  //Buffers
  BufferId AddBuffer( size_t size, const void* data = 0 );
  void RemoveBuffer(BufferId buffer);
  void UpdateBuffer(BufferId buffer, size_t size, void* data );
  void* MapBuffer( BufferId buffer, BufferMapMode mode );
  void UnmapBuffer();
  void BindVertexBuffer(BufferId buffer );
  void BindIndexBuffer(BufferId buffer );
  void BindUniformBuffer( BufferId bufferId, u32 bindingPoint );
  void BindShaderStorageBuffer( BufferId bufferId, u32 bindingPoint );

  //Textures
  TextureId Add2DTexture(const Image& image, bool generateMipmaps = true );
  TextureId Add2DArrayTexture(TextureFormat format, u32 width, u32 height, u32 layers, bool generateMipmaps = true);
  TextureId AddCubeTexture( Image* images, bool generateMipmaps = true );
  void RemoveTexture(TextureId textureId);
  void UpdateTexture(TextureId textureId );
  void Update2DArrayTexture( TextureId textureId, u32 layer, const Image& image );
  void UpdateCubeTexture( TextureId textureId, CubeTextureSide side, const Image& image );
  void Bind2DTexture( TextureId textureId, u32 textureUnit );
  void Bind2DArrayTexture( TextureId textureId, u32 textureUnit );
  void BindCubeTexture( TextureId textureId, u32 textureUnit );

  //Programs
  ProgramId AddProgram( const u8** vertexShaderSource, const u8** fragmentShaderSource );
  void RemoveProgram( u32 program );
  void UseProgram( ProgramId programId );
  ProgramId GetCurrentProgramId();
  s32 GetUniformLocation( ProgramId programId, const char* name );
  void SetUniform( s32 location, f32 value );
  void SetUniform( s32 location, s32 value );
  void SetUniform( s32 location, u32 value );
  void SetUniform( s32 location, const vec2& value );
  void SetUniform( s32 location, const uvec2& value );
  void SetUniform( s32 location, const vec3& value );
  void SetUniform( s32 location, vec3* value, u32 count );
  void SetUniform( s32 location, const mat4& value );

  //FrameBuffers
  FBOId AddFrameBuffer();
  void BindFrameBuffer( FBOId fbo );
  void ClearColorAttachment( u32 index, const vec4& value );
  void SetDrawBuffers( u32 n, u32* buffers );
  void RemoveFrameBuffer(FBOId fbo);
  void Attach2DColorTextureToFrameBuffer( FBOId fbo, u32 index, TextureId texture, u32 level = 0 );
  void AttachDepthStencilTextureToFrameBuffer( FBOId fbo, TextureId texture, u32 level = 0 );

  //VAOs
  VAOId AddVAO();
  void RemoveVAO(VAOId vao);
  void BindVAO( VAOId vao);

  //Meshes
  void SetupMeshVertexFormat( MeshId meshId );
  void SetupInstancedAttribute( AttributeType type, const AttributeDescription& description );
  MeshId AddMeshFromFile( const char* path, u32 submesh = 0 );
  u32 GetMeshCountFromFile( const char* path );
  void AddMultipleMeshesFromFile( const char* path, MeshId* meshId, Material* materials, u32 count );

  MeshId AddMesh( const Mesh& m );
  MeshId AddMesh( const void* vertexData, size_t vertexCount, VertexFormat vertexFormat, const unsigned int* index, size_t indexCount );

  MeshId CreateQuad( const uvec2& size, bool generateUV, bool generateNormals, const uvec2& subdivision = uvec2(1u,1u) );
  Mesh GetMesh( MeshId meshId );
  void DrawMesh( MeshId meshId );
  void DrawCall( u32 vertexCount );
  void DrawMeshInstanced( MeshId meshId, u32 instanceCount );
  MultiMeshId AddMultiMesh( const char* path );
  MultiMesh  GetMultiMesh( MultiMeshId id );
  void DrawMultiMesh( MultiMeshId id );

  //State
  void SetClearColor(const vec4& color);
  void SetClearDepth(f32 value);
  void ClearBuffers( u32 mask );
  void SetViewport( s32 x, s32 y, size_t width, size_t height );
  void SetBlendingMode(BlendingMode mode);
  void SetBlendingFunction(BlendingFunction sourceColor, BlendingFunction destinationColor, BlendingFunction sourceAlpha, BlendingFunction destinationAlpha );
  void SetCullFace( CullFace cullFace );
  void SetDepthTest( DepthTestFunction function );
  void DisableDepthWrite();
  void EnableDepthWrite();
  void Wired(bool wired);

  void PrintInfo();

private:

  std::vector<VAOId>  mVAO;
  std::vector<BufferId>    mBuffer;
  std::vector<size_t> mBufferSize;
  std::vector<TextureId>  mTexture;
  std::vector<vec3> mTextureSize;
  std::vector<ProgramId>  mProgram;
  std::vector<FBOId>  mFrameBuffer;
  std::vector<vec3> mFrameBufferSize;
  ComponentList<Mesh>* mMesh;
  ComponentList<MultiMesh>* mMultiMesh;

  s32   mCurrentProgram;
  s32   mCurrentVertexBuffer;
  s32   mCurrentIndexBuffer;
  s32   mCurrentVAO;
  FBOId mCurrentFBO;
  bool  mIsDepthTestEnabled;
  bool  mIsCullFaceEnabled;
  bool  mIsBlendingEnabled;
};


}
