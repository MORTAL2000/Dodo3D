
#include <render-manager.h>
#include <GL/glew.h>
#include <log.h>
#include <math.h>


#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifdef DEBUG
#define CHECK_GL_ERROR(a) (a); CheckGlError(#a)
#define PRINT_GLSL_COMPILER_LOG(a) PrintGLSLCompilerLog(a)
#else
#define CHECK_GL_ERROR(a) (a)
#define PRINT_GLSL_COMPILER_LOG(a)
#endif

#define MAX_MESH_COUNT 1000

using namespace Dodo;


namespace
{

struct GLErrorString
{
  const GLenum mError;
  const char*  mString;
};

GLErrorString gGlErrorString[] =
{
   { GL_NO_ERROR,           "GL_NO_ERROR" },
   { GL_INVALID_ENUM,       "GL_INVALID_ENUM" },
   { GL_INVALID_VALUE,      "GL_INVALID_VALUE" },
   { GL_INVALID_OPERATION,  "GL_INVALID_OPERATION" },
   { GL_OUT_OF_MEMORY,      "GL_OUT_OF_MEMORY" }
};

u32 gErrorCount = sizeof(gGlErrorString) / sizeof(gGlErrorString[0]);

const char* GLErrorToString( GLenum error )
{
  for( u32 i(0); i<gErrorCount; ++i)
  {
    if (error == gGlErrorString[i].mError)
    {
      return gGlErrorString[i].mString;
    }
  }
  return "Unknown OpenGL error";
}

void CheckGlError( const char* operation )
{
  GLint error = glGetError();
  if( error != GL_NO_ERROR )
  {
    std::cout<<"OpenGL error "<<GLErrorToString(error)<<" after "<<operation<<std::endl;
  }
}

GLenum GetGLType( Dodo::Type type )
{
  switch( type )
  {
    case Dodo::U8:
      return GL_UNSIGNED_BYTE;
    case Dodo::S8:
      return GL_BYTE;
    case Dodo::U32:
      return GL_UNSIGNED_INT;
    case Dodo::S32:
      return GL_INT;
    case Dodo::F32:
      return GL_FLOAT;
    default:
      return GL_FLOAT;
  }
}

void PrintGLSLCompilerLog(GLuint obj)
{
  int infologLength = 0;
  int maxLength;

  if(glIsShader(obj))
  {
    glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
    char* infoLog = new char[maxLength];
    glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
    if( infologLength > 0 )
    {
      DODO_LOG("GLSL Error: %s",infoLog );
    }

    delete[] infoLog;
  }
  else
  {
    glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
    char* infoLog = new char[maxLength];
    glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
    if( infologLength > 0 )
    {
      DODO_LOG( "GLSL Error: %s", infoLog );
    }

    delete[] infoLog;
  }
}

bool GetTextureGLFormat( TextureFormat format, GLenum& dataFormat, GLenum& internalFormat, GLenum& dataType )
{
  dataType = GL_UNSIGNED_BYTE;
  switch( format )
  {
    case Dodo::FORMAT_GL_DEPTH:
    {
      dataFormat =  GL_DEPTH_COMPONENT;
      internalFormat = GL_DEPTH_COMPONENT;
      break;
    }
    case Dodo::FORMAT_GL_DEPTH_STENCIL:
    {
      dataFormat = GL_DEPTH_STENCIL;
      internalFormat = GL_DEPTH24_STENCIL8;
      dataType = GL_UNSIGNED_INT_24_8;
      break;
    }
    case Dodo::FORMAT_RGB8:
    {
      dataFormat = GL_RGB;
      internalFormat = GL_RGB8;
      break;
    }
    case Dodo::FORMAT_RGBA8:
    {
      dataFormat = GL_RGBA;
      internalFormat = GL_RGBA8;
      break;
    }
    case Dodo::FORMAT_R8:
    {
      dataFormat = GL_LUMINANCE;
      internalFormat = GL_LUMINANCE8;
      break;
    }
    case Dodo::FORMAT_RGBA32F:
    {
      dataFormat = GL_RGBA;
      internalFormat = GL_RGBA32F;
      dataType = GL_FLOAT;
      break;
    }
    default:  //@TODO: Implement the rest
    {
      return false;
    }
  }

  return true;
}


GLenum GetGLPrimitive( u32 primitive )
{
  switch( primitive )
  {
    case PRIMITIVE_TRIANGLES:
    {
      return GL_TRIANGLES;
    }
    case PRIMITIVE_POINTS:
    {
      return GL_POINTS;
    }
    case PRIMITIVE_LINES:
    {
      return GL_LINES;
    }
    default:
    {
      return 0;
    }
  }
}

GLenum GetGLBlendingFunction( BlendingFunction function )
{
  switch( function )
  {
    case BLENDING_FUNCTION_ZERO:
      return GL_ZERO;
    case BLENDING_FUNCTION_ONE:
      return GL_ONE;
    case BLENDING_SOURCE_COLOR:
      return GL_SRC_COLOR;
    case BLENDING_ONE_MINUS_SOURCE_COLOR:
      return GL_ONE_MINUS_SRC_COLOR;
    case BLENDING_DESTINATION_COLOR:
      return GL_DST_COLOR;
    case BLENDING_ONE_MINUS_DESTINATION_COLOR:
      return GL_ONE_MINUS_DST_COLOR;
    case BLENDING_SOURCE_ALPHA:
      return GL_SRC_ALPHA;
    case BLENDING_ONE_MINUS_SOURCE_ALPHA:
      return GL_ONE_MINUS_SRC_ALPHA;
    case BLENDING_DESTINATION_ALPHA:
      return GL_DST_ALPHA;
    case BLENDING_ONE_MINUS_DESTINATION_ALPHA:
      return GL_ONE_MINUS_DST_ALPHA;
    default:
      return GL_ZERO;
  }
}

} //unnamed namespace
RenderManager::RenderManager()
:mMesh(0)
,mCurrentVertexBuffer(-1)
,mCurrentIndexBuffer(-1)
,mCurrentVAO(-1)
,mCurrentFBO(0)
,mIsDepthTestEnabled(false)
,mIsCullFaceEnabled(true)
,mIsBlendingEnabled(false)
{}

void RenderManager::Init()
{
  //Init glew
  glewExperimental = GL_TRUE;
  glewInit();

  mMesh = new ComponentList<Mesh>(MAX_MESH_COUNT);
  VAOId defaultVAO = AddVAO();
  BindVAO( defaultVAO );
}

RenderManager::~RenderManager()
{
  CHECK_GL_ERROR( glDeleteVertexArrays(mVAO.size(), mVAO.data() ) );
  CHECK_GL_ERROR( glDeleteBuffers( mBuffer.size(), mBuffer.data() ) );
  CHECK_GL_ERROR( glDeleteTextures( mTexture.size(), mTexture.data()) );
  CHECK_GL_ERROR( glDeleteFramebuffers( mFrameBuffer.size(), mFrameBuffer.data() ) );

  for( u32 i(0); i<mProgram.size(); ++i )
  {
    CHECK_GL_ERROR( glDeleteProgram( mProgram[i] ) );
  }
}

BufferId RenderManager::AddBuffer( size_t size, const void* data )
{
  BufferId buffer(0);
  CHECK_GL_ERROR( glGenBuffers( 1, &buffer) );
  CHECK_GL_ERROR( glBindBuffer( GL_COPY_WRITE_BUFFER, buffer ) );
  CHECK_GL_ERROR( glBufferData( GL_COPY_WRITE_BUFFER, size, data, GL_STATIC_DRAW ) );

  mBuffer.push_back( buffer );
  mBufferSize.push_back( size );

  return buffer;
}

void RenderManager::RemoveBuffer(BufferId buffer)
{
  for( u32 i(0); i<mBuffer.size(); ++i )
  {
    if( mBuffer[i] == buffer )
    {
      mBuffer.erase( mBuffer.begin()+i);
      mBufferSize.erase( mBufferSize.begin()+i);

      CHECK_GL_ERROR( glDeleteBuffers( 1, &buffer ) );
    }
  }
}

void RenderManager::UpdateBuffer(BufferId buffer, size_t size, void* data )
{
  for( u32 i(0); i<mBuffer.size(); ++i )
  {
    if( mBuffer[i] == buffer )
    {
      if( mBufferSize[i] < size )
      {
        //Not enoguh room
        CHECK_GL_ERROR( glBindBuffer( GL_COPY_WRITE_BUFFER, buffer ) );
        CHECK_GL_ERROR( glBufferData( GL_COPY_WRITE_BUFFER, size, data, GL_STATIC_DRAW ) );
        mBufferSize[i] = size;
      }
      else
      {
        CHECK_GL_ERROR( glBindBuffer( GL_COPY_WRITE_BUFFER, buffer ) );
        CHECK_GL_ERROR( glBufferSubData( GL_COPY_WRITE_BUFFER, 0, size, data ) );
      }
    }
  }
}

void* RenderManager::MapBuffer( BufferId buffer, BufferMapMode mode )
{
  CHECK_GL_ERROR( glBindBuffer( GL_COPY_WRITE_BUFFER, buffer ) );

  void* result(0);
  switch( mode )
  {
    case BUFFER_MAP_READ:
      result = CHECK_GL_ERROR( glMapBuffer( GL_COPY_WRITE_BUFFER, GL_READ_ONLY ) );
      break;
    case BUFFER_MAP_WRITE:
      result = CHECK_GL_ERROR( glMapBuffer( GL_COPY_WRITE_BUFFER, GL_WRITE_ONLY ) );
      break;
    case BUFFER_MAP_READ_WRITE:
      result = CHECK_GL_ERROR( glMapBuffer( GL_COPY_WRITE_BUFFER, GL_READ_WRITE ) );
      break;
  }

  return result;
}

void RenderManager::UnmapBuffer()
{
  CHECK_GL_ERROR( glUnmapBuffer( GL_COPY_WRITE_BUFFER ) );
}

void RenderManager::BindVertexBuffer(BufferId buffer )
{
  if( mCurrentVertexBuffer == -1 || (u32)mCurrentVertexBuffer != buffer )
  {
    CHECK_GL_ERROR( glBindBuffer( GL_ARRAY_BUFFER, buffer ) );
    mCurrentVertexBuffer = buffer;
  }
}

void RenderManager::BindIndexBuffer(BufferId buffer )
{
  if( mCurrentIndexBuffer == -1 || (u32)mCurrentIndexBuffer != buffer )
  {
    CHECK_GL_ERROR( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer ) );
    mCurrentIndexBuffer = buffer;
  }
}

void RenderManager::BindUniformBuffer( BufferId bufferId, u32 bindingPoint )
{
  CHECK_GL_ERROR( glBindBufferBase( GL_UNIFORM_BUFFER, bindingPoint, bufferId ) );
}

void RenderManager::BindShaderStorageBuffer( BufferId bufferId, u32 bindingPoint  )
{
  CHECK_GL_ERROR( glBindBufferBase( GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferId ) );
}

TextureId RenderManager::Add2DTexture(const Image& image, bool generateMipmaps)
{
  TextureId texture;

  GLenum dataFormat,internalFormat,glDataType;
  if( !GetTextureGLFormat( image.mFormat, dataFormat, internalFormat, glDataType ) )
  {
    DODO_LOG("Error: Unsupported texture format");
    return texture;
  }

  CHECK_GL_ERROR( glGenTextures(1, &texture ) );
  CHECK_GL_ERROR( glBindTexture(GL_TEXTURE_2D, texture) );

  u32 numberOfMipmaps = floor( log2(image.mWidth > image.mHeight ? image.mWidth : image.mHeight) ) + 1;
  CHECK_GL_ERROR( glTexStorage2D( GL_TEXTURE_2D, numberOfMipmaps, internalFormat, image.mWidth, image.mHeight ) );
  if( image.mData )
  {
    CHECK_GL_ERROR( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, image.mWidth, image.mHeight, dataFormat, glDataType, image.mData ) );
  }

  if( generateMipmaps )
  {
    glGenerateMipmap( GL_TEXTURE_2D );
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  }

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

  mTexture.push_back(texture);
  mTextureSize.push_back( vec3( image.mWidth, image.mHeight, 0.0f ));

  return texture;
}

TextureId RenderManager::Add2DArrayTexture(TextureFormat format, u32 width, u32 height, u32 layers, bool generateMipmaps)
{
  TextureId texture;
  GLenum dataFormat,internalFormat,glDataType;
  if( !GetTextureGLFormat( format, dataFormat, internalFormat, glDataType ) )
  {
    DODO_LOG("Error: Unsupported texture format");
    return texture;
  }

  glGenTextures( 1, &texture );
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

  u32 numberOfMipmaps = floor( log2(width > height ? width : height) ) + 1;
  //Create storage for the texture. (100 layers of 1x1 texels)
  glTexStorage3D( GL_TEXTURE_2D_ARRAY,
                  numberOfMipmaps,
                  internalFormat,
                  width,height,layers );

  if( generateMipmaps )
  {
    glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
  }

  mTexture.push_back(texture);
  mTextureSize.push_back( vec3( width, height, layers ));

  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

  return texture;
}

TextureId RenderManager::AddCubeTexture( Image* images, bool generateMipmaps )
{
  TextureId texture;
  GLenum dataFormat,internalFormat,glDataType;
  if( !GetTextureGLFormat( images[0].mFormat, dataFormat, internalFormat, glDataType ) )
  {
    DODO_LOG("Error: Unsupported texture format");
    return texture;
  }

  CHECK_GL_ERROR(glGenTextures(1, &texture));
  CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, texture));

  for( u8 i(0); i<6; ++i )
  {
    CHECK_GL_ERROR( glTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, images[i].mWidth, images[i].mHeight, 0, dataFormat, glDataType, images[i].mData ) );
  }

  if( generateMipmaps )
  {
    glGenerateMipmap( GL_TEXTURE_CUBE_MAP );
  }

  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));

  return texture;

}

void RenderManager::RemoveTexture(TextureId textureId)
{
  for( u32 i(0); i<mTexture.size(); ++i )
  {
    if( mTexture[i] == textureId )
    {
      mTexture.erase( mTexture.begin()+i);
      mTextureSize.erase( mTextureSize.begin()+i);
      CHECK_GL_ERROR( glDeleteTextures( 1, &textureId) );
    }
  }
}

void RenderManager::UpdateTexture(TextureId textureId )
{}

void RenderManager::Update2DArrayTexture( TextureId textureId, u32 layer, const Image& image )
{
  GLenum dataFormat,internalFormat,glDataType;
  if( !GetTextureGLFormat( image.mFormat, dataFormat, internalFormat, glDataType ) )
  {
    DODO_LOG("Error: Wrong texture format");
    return;
  }

  glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);

  glTexSubImage3D( GL_TEXTURE_2D_ARRAY,
                   0,
                   0,0,layer,
                   image.mWidth,image.mHeight,1,
                   dataFormat,
                   glDataType,
                   image.mData);
}

void RenderManager::UpdateCubeTexture( TextureId textureId, CubeTextureSide side, const Image& image )
{}

void RenderManager::Bind2DTexture( TextureId textureId, u32 textureUnit )
{
  CHECK_GL_ERROR( glActiveTexture( GL_TEXTURE0 + textureUnit) );
  CHECK_GL_ERROR( glBindTexture(GL_TEXTURE_2D, textureId) );
}

void RenderManager::Bind2DArrayTexture( TextureId textureId, u32 textureUnit )
{
  CHECK_GL_ERROR( glActiveTexture( GL_TEXTURE0 + textureUnit) );
  CHECK_GL_ERROR( glBindTexture(GL_TEXTURE_2D_ARRAY, textureId) );

}

void RenderManager::BindCubeTexture( TextureId textureId, u32 textureUnit )
{
  //if( mCurrent2DArrayTexture == -1 || (u32)mCurrent2DArrayTexture != textureId )
  {
    CHECK_GL_ERROR( glActiveTexture( GL_TEXTURE0 + textureUnit) );
    CHECK_GL_ERROR( glBindTexture(GL_TEXTURE_CUBE_MAP, textureId) );
  }
}


ProgramId RenderManager::AddProgram( const u8** vertexShaderSource, const u8** fragmentShaderSource )
{
  //Compile vertex shader
  GLuint vertexShader = CHECK_GL_ERROR( glCreateShader( GL_VERTEX_SHADER ));
  CHECK_GL_ERROR( glShaderSource( vertexShader, 1, (const GLchar**)vertexShaderSource, NULL ) );
  CHECK_GL_ERROR( glCompileShader( vertexShader ) );
  PRINT_GLSL_COMPILER_LOG( vertexShader );

  //Compile fragment shader
  GLuint fragmentShader = CHECK_GL_ERROR( glCreateShader( GL_FRAGMENT_SHADER ) );
  CHECK_GL_ERROR( glShaderSource( fragmentShader, 1, (const GLchar**)fragmentShaderSource, NULL ) );
  CHECK_GL_ERROR( glCompileShader( fragmentShader ) );
  PRINT_GLSL_COMPILER_LOG( fragmentShader );

  //Link vertex and fragment shader together
  ProgramId program = CHECK_GL_ERROR( glCreateProgram() );
  PrintGLSLCompilerLog( program );
  CHECK_GL_ERROR( glAttachShader( program, vertexShader ) );
  CHECK_GL_ERROR( glAttachShader( program, fragmentShader ) );
  CHECK_GL_ERROR( glLinkProgram( program ) );

  //Delete shaders objects
  CHECK_GL_ERROR( glDeleteShader( vertexShader ) );
  CHECK_GL_ERROR( glDeleteShader( fragmentShader ) );

  mProgram.push_back(program);
  return program;
}

void RenderManager::RemoveProgram( u32 program )
{
  for( u32 i(0); i<mProgram.size(); ++i )
  {
    if( mProgram[i] == program )
    {
      mProgram.erase( mProgram.begin()+i);
      CHECK_GL_ERROR( glDeleteProgram( program ) );
    }
  }
}

void RenderManager::UseProgram( ProgramId programId )
{
  CHECK_GL_ERROR( glUseProgram( programId ) );
}

s32 RenderManager::GetUniformLocation( ProgramId programId, const char* name )
{
  s32 location = CHECK_GL_ERROR( glGetUniformLocation( programId, name ) );
  return location;
}

void RenderManager::SetUniform( s32 location, f32 value )
{
  CHECK_GL_ERROR( glUniform1f( location, value ) );
}

void RenderManager::SetUniform( s32 location, s32 value )
{
  CHECK_GL_ERROR( glUniform1i( location, value ) );
}

void RenderManager::SetUniform( s32 location, u32 value )
{
  CHECK_GL_ERROR( glUniform1ui( location, value ) );
}

void RenderManager::SetUniform( s32 location, const vec2& value )
{
  CHECK_GL_ERROR( glUniform2fv( location, 1, value.data ) );
}

void RenderManager::SetUniform( s32 location, const uvec2& value )
{
  CHECK_GL_ERROR( glUniform2uiv( location, 1, value.data ) );
}

void RenderManager::SetUniform( s32 location, const vec3& value )
{
  CHECK_GL_ERROR( glUniform3fv( location, 1, value.data ) );
}

void RenderManager::SetUniform( s32 location, const mat4& value )
{
  CHECK_GL_ERROR( glUniformMatrix4fv( location, 1, GL_FALSE, value.data ) );
}

FBOId RenderManager::AddFrameBuffer()
{
  FBOId fbo(0);
  CHECK_GL_ERROR( glGenFramebuffers( 1, &fbo ) );
  mFrameBuffer.push_back( fbo );
  return fbo;
}

void RenderManager::RemoveFrameBuffer(FBOId fboId )
{
  for( u32 i(0); i<mFrameBuffer.size(); ++i )
  {
    if( mFrameBuffer[i] == fboId )
    {
      mFrameBuffer.erase( mFrameBuffer.begin()+i);
      CHECK_GL_ERROR( glDeleteFramebuffers( 1, &fboId ) );
    }
  }
}

void RenderManager::BindFrameBuffer( FBOId fbo )
{
  if( mCurrentFBO != fbo )
  {
    CHECK_GL_ERROR( glBindFramebuffer( GL_FRAMEBUFFER, fbo ) );
    mCurrentFBO = fbo;
  }
}

void RenderManager::SetDrawBuffers( u32 n, u32* buffers )
{
  GLenum* buffer = new GLenum[n];
  for( u32 i(0); i<n; ++i )
  {
    buffer[i] = GL_COLOR_ATTACHMENT0 + buffers[i];
  }

  CHECK_GL_ERROR( glDrawBuffers( n, buffer ) );
  delete[] buffer;
}

void RenderManager::Attach2DColorTextureToFrameBuffer( FBOId fbo, u32 index, TextureId texture, u32 level )
{
  FBOId currentFBO = mCurrentFBO;
  BindFrameBuffer( fbo );
  CHECK_GL_ERROR( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture, level  ) );
  BindFrameBuffer( currentFBO );
}

void RenderManager::AttachDepthStencilTextureToFrameBuffer( FBOId fbo, TextureId texture, u32 level )
{
  FBOId currentFBO = mCurrentFBO;
  BindFrameBuffer( fbo );
  CHECK_GL_ERROR( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, level  ) );
  BindFrameBuffer( currentFBO );
}

void RenderManager::ClearColorAttachment( u32 index, const vec4& value )
{
  CHECK_GL_ERROR( glClearBufferfv( GL_COLOR, index, value.data ));
}

VAOId RenderManager::AddVAO()
{
  VAOId vao(0);
  CHECK_GL_ERROR( glGenVertexArrays(1, &vao) );

  mVAO.push_back( vao );
  return vao;
}

void RenderManager::RemoveVAO(VAOId vao)
{
  for( u32 i(0); i<mVAO.size(); ++i )
  {
    if( mVAO[i] == vao )
    {
      mVAO.erase( mVAO.begin()+i);
      CHECK_GL_ERROR( glDeleteVertexArrays(1, &vao) );
    }
  }
}

void RenderManager::BindVAO( VAOId vao)
{
  CHECK_GL_ERROR( glBindVertexArray(vao) );
}


MeshId RenderManager::AddMesh( const char* path )
{
  Mesh newMesh;

  //Load mesh
  const struct aiScene* scene = NULL;
  scene = aiImportFile(path,aiProcessPreset_TargetRealtime_MaxQuality);
  if( !scene || scene->mNumMeshes <= 0 )
  {
    DODO_LOG("Error loading mesh %s", path );
    return INVALID_ID;
  }

  u32 i(0);
  //for (u32 i(0); i < scene->mNumMeshes; ++i)    //@TODO: Support submeshes
  {
    const struct aiMesh* mesh = scene->mMeshes[i];
    newMesh.mVertexCount = mesh->mNumVertices;

    bool bHasUV( mesh->HasTextureCoords(0) );
    bool bHasColor( mesh->HasVertexColors(0) );
    bool bHasNormal(mesh->HasNormals() );

    u32 vertexSize = 3;
    if( bHasUV)
      vertexSize += 2;
    if( bHasColor )
      vertexSize += 4;
    if(bHasNormal)
      vertexSize += 3;

    size_t vertexBufferSize( newMesh.mVertexCount * vertexSize * sizeof(f32) );
    f32* vertexData = new f32[ newMesh.mVertexCount * vertexSize ];
    u32 index(0);
    vec3 vertexMin( F32_MAX, F32_MAX, F32_MAX );
    vec3 vertexMax( 0.0f, 0.0f, 0.0f );
    for( u32 vertex(0); vertex<newMesh.mVertexCount; ++vertex )
    {
      vertexData[index++] = mesh->mVertices[vertex].x;
      vertexData[index++] = mesh->mVertices[vertex].y;
      vertexData[index++] = mesh->mVertices[vertex].z;

      //Find min and max vertex values
      if( mesh->mVertices[vertex].x < vertexMin.x )
      {
        vertexMin.x = mesh->mVertices[vertex].x;
      }
      if( mesh->mVertices[vertex].x > vertexMax.x )
      {
        vertexMax.x = mesh->mVertices[vertex].x;
      }
      if( mesh->mVertices[vertex].y < vertexMin.y )
      {
        vertexMin.y = mesh->mVertices[vertex].y;
      }
      if( mesh->mVertices[vertex].y > vertexMax.y )
      {
        vertexMax.y = mesh->mVertices[vertex].y;
      }
      if( mesh->mVertices[vertex].z < vertexMin.z )
      {
        vertexMin.z = mesh->mVertices[vertex].z;
      }
      if( mesh->mVertices[vertex].z > vertexMax.z )
      {
        vertexMax.z = mesh->mVertices[vertex].z;
      }

      if( bHasUV )
      {
        vertexData[index++] = mesh->mTextureCoords[0][vertex].x;
        vertexData[index++] = mesh->mTextureCoords[0][vertex].y;
      }

      if( bHasColor )
      {
        vertexData[index++] = mesh->mColors[0][vertex].r;
        vertexData[index++] = mesh->mColors[0][vertex].g;
        vertexData[index++] = mesh->mColors[0][vertex].b;
        vertexData[index++] = mesh->mColors[0][vertex].a;
      }

      if( bHasNormal )
      {
        vertexData[index++] = mesh->mNormals[vertex].x;
        vertexData[index++] = mesh->mNormals[vertex].y;
        vertexData[index++] = mesh->mNormals[vertex].z;
      }
    }

    u32* indices(0);
    size_t indexBufferSize(0);
    if( mesh->HasFaces() )
    {
      newMesh.mIndexCount = mesh->mNumFaces * 3; //@WARNING: Assuming triangles!
      indexBufferSize = newMesh.mIndexCount * sizeof( unsigned int );
      indices = new u32[newMesh.mIndexCount];
      for( u32 face(0); face<mesh->mNumFaces; ++face )
      {
        indices[face*3] = mesh->mFaces[face].mIndices[0];
        indices[face*3 + 1] = mesh->mFaces[face].mIndices[1];
        indices[face*3 + 2] = mesh->mFaces[face].mIndices[2];
      }
    }

    //Compute AABB
    DODO_LOG( "Min: (%f,%f,%f). Max: (%f,%f,%f) ", vertexMin.x, vertexMin.y, vertexMin.z,
              vertexMax.x, vertexMax.y,vertexMax.z);

    newMesh.mAABB.mCenter = 0.5f * ( vertexMax + vertexMin );
    newMesh.mAABB.mExtents = newMesh.mAABB.mCenter - vertexMin;
    DODO_LOG( "Center: (%f,%f,%f). Extents: (%f,%f,%f) ", newMesh.mAABB.mCenter.x, newMesh.mAABB.mCenter.y, newMesh.mAABB.mCenter.z,
                                                          newMesh.mAABB.mExtents.x, newMesh.mAABB.mExtents.y,newMesh.mAABB.mExtents.z);
    //Create buffers
    newMesh.mVertexBuffer = AddBuffer( vertexBufferSize, vertexData );
    delete[] vertexData;

    if( indices )
    {
      newMesh.mIndexBuffer = AddBuffer( indexBufferSize, indices );
      delete[] indices;
    }

    newMesh.mVertexFormat.SetAttribute(VERTEX_POSITION, AttributeDescription( F32x3, 0, vertexSize) );
    if( bHasUV )
    {
      newMesh.mVertexFormat.SetAttribute(VERTEX_UV, AttributeDescription( F32x2, newMesh.mVertexFormat.VertexSize(), vertexSize) );
    }
    if( bHasColor )
    {
      newMesh.mVertexFormat.SetAttribute(VERTEX_COLOR, AttributeDescription( F32x3, newMesh.mVertexFormat.VertexSize(), vertexSize) );
    }
    if( bHasNormal )
    {
      newMesh.mVertexFormat.SetAttribute(VERTEX_NORMAL, AttributeDescription( F32x3, newMesh.mVertexFormat.VertexSize(), vertexSize) );
    }
  }

  MeshId meshId( mMesh->Add( newMesh ) );
  return meshId;
}

MeshId RenderManager::AddMesh( const void* vertexData, size_t vertexCount, VertexFormat vertexFormat, const unsigned int* index, size_t indexCount )
{
  Mesh newMesh;

  if( vertexData != 0 )
  {
    size_t vertexBufferSize = vertexCount * vertexFormat.VertexSize();
    newMesh.mVertexCount = vertexCount;
    newMesh.mVertexBuffer = AddBuffer( vertexBufferSize, vertexData );
  }

  if( index != 0 )
  {
    size_t indexBufferSize = indexCount * sizeof( unsigned int );
    newMesh.mIndexCount = indexCount;
    newMesh.mIndexBuffer = AddBuffer( indexBufferSize, index );
  }

  newMesh.mVertexFormat = vertexFormat;

  //TO DO: Compute AABB
  MeshId meshId( mMesh->Add( newMesh ) );
  return meshId;
}

MeshId RenderManager::CreateQuad( const uvec2& size, bool generateUV, bool generateNormals, const uvec2& subdivision )
{
  size_t vertexCount = (subdivision.x + 1)*(subdivision.y + 1);

  size_t normalOffset(3);
  u32 vertexSize = 3;
  if( generateUV )
  {
    vertexSize += 2;
    normalOffset = 5;
  }
  if(generateNormals)
    vertexSize += 3;

  f32* vertexData = new f32[ vertexCount * vertexSize ];

  f32 deltaX = size.x / (f32)subdivision.x;
  f32 deltaY = size.y / (f32)subdivision.y;
  f32 deltaU = 1.0f/(f32)subdivision.x;
  f32 deltaV = 1.0f/(f32)subdivision.y;

  f32 x = -(f32)size.x * 0.5f;
  f32 y = -(f32)size.y * 0.5f;
  f32 u = 0.0f;
  f32 v = 0.0f;
  for(u32 i(0); i<subdivision.y+1; ++i )
  {
    x = -(f32)size.x * 0.5f;
    u = 0.0f;
    for( u32 j(0); j<subdivision.x+1; ++j )
    {
      float* vertex = vertexData + ( (i*(subdivision.x+1) + j) )*vertexSize;
      vec3* position = (vec3*)vertex;
      *position = Dodo::vec3(x,y,0.0f);
      if( generateUV )
      {
        vec2* texCoord = (vec2*)(vertex+3);
        *texCoord = Dodo::vec2(u,v);
      }
      if(generateNormals)
      {
        vec3* n = (vec3*)(vertex+normalOffset);
        *n = Dodo::vec3(0.0f,0.0f,1.0f);
      }
      x += deltaX;
      u += deltaU;
    }
    y += deltaY;
    v += deltaV;
  }

  u32* index = new u32[6*subdivision.x*subdivision.y];
  u32 current = 0;
  for(u32 i(0); i<6*subdivision.x*subdivision.y; i+=6 )
  {
    index[i] = current;
    index[i+1] = current+1;
    index[i+2] = current+subdivision.x+1;
    index[i+3] = current+subdivision.x+1;
    index[i+4] = current+1;
    index[i+5] = current+subdivision.x+2;

    current++;
    if( (current % (subdivision.x+1) ) == subdivision.x )
      current++;
  }

  Dodo::VertexFormat vertexFormat;
  vertexFormat.SetAttribute(VERTEX_POSITION, AttributeDescription( F32x3, 0, vertexSize) );
  if( generateUV )
  {
    vertexFormat.SetAttribute(VERTEX_UV, AttributeDescription( F32x2, vertexFormat.VertexSize(), vertexSize) );
  }
  if( generateNormals )
  {
    vertexFormat.SetAttribute(VERTEX_NORMAL, AttributeDescription( F32x3, vertexFormat.VertexSize(), vertexSize) );
  }

  Dodo::MeshId meshId = AddMesh( vertexData, vertexCount, vertexFormat, index, 6*subdivision.x*subdivision.y);
  delete[] vertexData;
  delete[] index;

  return meshId;
}

Mesh RenderManager::GetMesh( MeshId meshId )
{
  Mesh* mesh = mMesh->GetElement(meshId);
  return *mesh;
}

void RenderManager::SetupMeshVertexFormat( MeshId meshId )
{
  Mesh* mesh = mMesh->GetElement(meshId);
  if( !mesh )
  {
    DODO_LOG("Error: Invalid Mesh id");
    return;
  }

  const VertexFormat& vertexFormat( mesh->mVertexFormat );
  u32 attributeCount( vertexFormat.AttributeCount() );
  size_t vertexSize( vertexFormat.VertexSize() );

  BindVertexBuffer( mesh->mVertexBuffer );
  for( u32 i(0); i<attributeCount; ++i )
  {
    if( vertexFormat.IsAttributeEnabled( i ) )
    {
      const AttributeDescription& attributeDescription = vertexFormat.GetAttributeDescription(i);
      CHECK_GL_ERROR( glEnableVertexAttribArray(i) );
      CHECK_GL_ERROR( glVertexAttribPointer(i,
                                            TypeElementCount(attributeDescription.mComponentType),
                                            GetGLType(attributeDescription.mComponentType),
                                            false,
                                            vertexSize,
                                            (void*)attributeDescription.mOffset ) );
    }
    else
    {
      CHECK_GL_ERROR( glDisableVertexAttribArray(i) );
    }
  }
}

void RenderManager::SetupInstancedAttribute( AttributeType type, const AttributeDescription& description )
{
  BindVertexBuffer( description.mBuffer );
  CHECK_GL_ERROR( glEnableVertexAttribArray(type) );
  CHECK_GL_ERROR( glVertexAttribPointer(type,
                                        TypeElementCount(description.mComponentType),
                                        GetGLType(description.mComponentType),
                                        false,
                                        description.mStride,
                                        (void*)description.mOffset ) );
  CHECK_GL_ERROR( glVertexAttribDivisor( type, description.mDivisor));
}

void RenderManager::DrawMesh( MeshId meshId )
{
  Mesh* mesh = mMesh->GetElement(meshId);
  if( !mesh )
  {
    DODO_LOG("Error: Invalid Mesh id");
    return;
  }

  GLenum primitive = GetGLPrimitive( mesh->mPrimitive );
  if( !primitive )
  {
    DODO_LOG("DrawUnIndexed error - Wrong primtive type");
    return;
  }

  if( mesh->mIndexBuffer )
  {
    BindIndexBuffer( mesh->mIndexBuffer );
    CHECK_GL_ERROR( glDrawElements( primitive, mesh->mIndexCount, GL_UNSIGNED_INT, (GLvoid*)0) );
  }
  else
  {
    CHECK_GL_ERROR( glDrawArrays( primitive, 0, mesh->mVertexCount ) );
  }
}

void RenderManager::DrawMeshInstanced( MeshId meshId, u32 instanceCount )
{
  Mesh* mesh = mMesh->GetElement(meshId);
  if( !mesh )
  {
    DODO_LOG("Error: Invalid Mesh id");
    return;
  }

  GLenum primitive = GetGLPrimitive( mesh->mPrimitive );
  if( !primitive )
  {
    DODO_LOG("DrawUnIndexed error - Wrong primtive type");
    return;
  }

  if( mesh->mIndexBuffer )
  {
    BindIndexBuffer( mesh->mIndexBuffer );
    CHECK_GL_ERROR( glDrawElementsInstanced( primitive, mesh->mIndexCount, GL_UNSIGNED_INT, (GLvoid*)0, instanceCount) );
  }
  else
  {
    CHECK_GL_ERROR( glDrawArraysInstanced( primitive, 0, mesh->mVertexCount, instanceCount ) );
  }
}

void RenderManager::SetClearColor(const vec4& color)
{
  CHECK_GL_ERROR( glClearColor( color.x, color.y, color.z, color.w ) );
}

void RenderManager::SetClearDepth(f32 value)
{
  CHECK_GL_ERROR( glClearDepth(value) );
}

void RenderManager::ClearBuffers( u32 mask )
{
  GLbitfield buffers(0);

  if( mask & COLOR_BUFFER )
    buffers |= GL_COLOR_BUFFER_BIT;

  if( mask & DEPTH_BUFFER )
    buffers |= GL_DEPTH_BUFFER_BIT;

  if( mask & STENCIL_BUFFER )
    buffers |= GL_STENCIL_BUFFER_BIT;


  CHECK_GL_ERROR( glClear( buffers ) );
}

void RenderManager::SetViewport( s32 x, s32 y, size_t width, size_t height )
{
  CHECK_GL_ERROR( glViewport( x, y, width, height ) );
}


void RenderManager::SetCullFace( CullFace cullFace )
{
  if( cullFace == CULL_NONE )
  {
    if( mIsCullFaceEnabled )
    {
      CHECK_GL_ERROR(glDisable(GL_CULL_FACE));
      mIsCullFaceEnabled = false;
    }
  }
  else
  {
    if( !mIsCullFaceEnabled )
    {
      CHECK_GL_ERROR(glEnable(GL_CULL_FACE));
      mIsCullFaceEnabled = true;
    }

    if( cullFace == CULL_FRONT )
    {
      CHECK_GL_ERROR(glCullFace( GL_FRONT ));
    }
    else if(cullFace == CULL_BACK )
    {
      CHECK_GL_ERROR(glCullFace( GL_BACK ));
    }
    else if(cullFace == CULL_FRONT_BACK )
    {
      CHECK_GL_ERROR(glCullFace( GL_FRONT_AND_BACK ));
    }
  }
}

void RenderManager::SetDepthTest( DepthTestFunction function)
{
  if( function == DEPTH_TEST_DISABLED  )
  {
    if( mIsDepthTestEnabled )
    {
      CHECK_GL_ERROR(glDisable(GL_DEPTH_TEST));
      mIsDepthTestEnabled = false;
    }
  }
  else
  {
    if( !mIsDepthTestEnabled )
    {
      CHECK_GL_ERROR(glEnable(GL_DEPTH_TEST));
      mIsDepthTestEnabled = true;
    }

    switch( function )
    {
      case DEPTH_TEST_NEVER:
        CHECK_GL_ERROR(glDepthFunc(GL_NEVER));
        break;

      case DEPTH_TEST_ALWAYS:
        CHECK_GL_ERROR(glDepthFunc(GL_ALWAYS));
        break;

      case DEPTH_TEST_LESS_EQUAL:
        CHECK_GL_ERROR(glDepthFunc(GL_LEQUAL));
        break;

      case DEPTH_TEST_LESS:
        CHECK_GL_ERROR(glDepthFunc(GL_LESS));
        break;

      case DEPTH_TEST_GREATER_EQUAL:
        CHECK_GL_ERROR(glDepthFunc(GL_GEQUAL));
        break;

      case DEPTH_TEST_GREATER:
        CHECK_GL_ERROR(glDepthFunc(GL_GREATER));
        break;

      case DEPTH_TEST_EQUAL:
        CHECK_GL_ERROR(glDepthFunc(GL_EQUAL));
        break;

      case DEPTH_TEST_NOT_EQUAL:
        CHECK_GL_ERROR(glDepthFunc(GL_NOTEQUAL));
        break;

      default:
        break;
    }
  }
}
void RenderManager::DisableDepthWrite()
{
  CHECK_GL_ERROR(glDepthMask(false));
}

void RenderManager::EnableDepthWrite()
{
  CHECK_GL_ERROR(glDepthMask(true));
}

void RenderManager::Wired( bool wired)
{
  if( wired )
  {
    CHECK_GL_ERROR(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));
  }
  else
  {
    CHECK_GL_ERROR(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ));
  }
}

void RenderManager::SetBlendingMode(BlendingMode mode)
{
  if( mode == BLEND_DISABLED )
  {
    if( mIsBlendingEnabled )
    {
      CHECK_GL_ERROR(glDisable(GL_BLEND));
      mIsBlendingEnabled = false;
    }
  }
  else
  {
    if( !mIsBlendingEnabled )
    {
      CHECK_GL_ERROR(glEnable(GL_BLEND));
      mIsBlendingEnabled = true;
    }

    switch( mode )
    {
      case BLEND_ADD:
        CHECK_GL_ERROR(glBlendEquation(GL_FUNC_ADD));
        break;
      case BLEND_SUBTRACT:
        CHECK_GL_ERROR(glBlendEquation(GL_FUNC_SUBTRACT));
        break;
      case BLEND_REVERSE_SUBTRACT:
        CHECK_GL_ERROR(glBlendEquation(GL_FUNC_REVERSE_SUBTRACT));
        break;
      case BLEND_MIN:
        CHECK_GL_ERROR(glBlendEquation(GL_MIN));
        break;
      case BLEND_MAX:
        CHECK_GL_ERROR(glBlendEquation(GL_MAX));
        break;
      default:
        break;
    }
  }
}

void RenderManager::SetBlendingFunction(BlendingFunction sourceColor, BlendingFunction destinationColor, BlendingFunction sourceAlpha, BlendingFunction destinationAlpha )
{
  CHECK_GL_ERROR( glBlendFuncSeparate( GetGLBlendingFunction(sourceColor),
                                       GetGLBlendingFunction(destinationColor),
                                       GetGLBlendingFunction(sourceAlpha),
                                       GetGLBlendingFunction(destinationAlpha)
                                     ) );
}


void RenderManager::PrintInfo()
{
  const GLubyte* glVersion( glGetString(GL_VERSION) );
  const GLubyte* glslVersion( glGetString(  GL_SHADING_LANGUAGE_VERSION) );
  const GLubyte* glRenderer = glGetString(GL_RENDERER);

  DODO_LOG( "GL %s", (char*)glVersion );
  DODO_LOG( "GLSL %s", (char*)glslVersion );
  DODO_LOG( "%s", (char*)glRenderer );
}
