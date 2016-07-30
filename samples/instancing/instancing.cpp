
#include <gl-application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>
#include <gl-renderer.h>
#include <cstdlib>

using namespace Dodo;
namespace
{

u32 gFrameCount = 0u;

const char* gVertexShader[] = {
                               "#version 440 core\n"
                               "#extension GL_EXT_gpu_shader4: enable\n"
                               "layout (location = 0 ) in vec3 aPosition;\n"
                               "layout (location = 4 ) in vec4 aInstanceColor;\n"
                               "layout (std140, binding=0) buffer ModelMatrices\n"
                               "{\n "
                               "  mat4 uModelMatrix[];\n"
                               "};\n"
                               "out vec4 color;\n"
                               "void main(void)\n"
                               "{\n"
                               "  gl_Position = uModelMatrix[gl_InstanceID] * vec4(aPosition,1.0);\n"
                               "  color = aInstanceColor;\n"
                               "}\n"
};
const char* gFragmentShader[] = {
                                 "#version 440 core\n"
                                 "out vec4 outputColor;\n"
                                 "in vec4 color;\n"
                                 "void main(void)\n"
                                 "{\n"
                                 "  outputColor = color;\n"
                                 "}\n"
};

u32 gQuadCount = 500.0f;
}


class App : public Dodo::GLApplication
{
public:
  App()
:Dodo::GLApplication("Demo",500,500,4,4)
  {}

  ~App()
  {
    DODO_LOG("%u", gFrameCount );
  }

  void Init()
  {
    mQuad = mRenderer.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Instanced attribute
    vec4* instanceColor = new vec4[gQuadCount*gQuadCount];
    for( u32 i(0); i != gQuadCount*gQuadCount; ++i )
    {
      instanceColor[i] = vec4( (rand()%255)/255.0f,(rand()%255)/255.0f,(rand()%255)/255.0f, 1.0f );
    }
    mInstancedBuffer = mRenderer.AddBuffer( sizeof(vec4)*gQuadCount*gQuadCount, instanceColor );
    delete[] instanceColor;
    mInstancedAttribute = AttributeDescription( Dodo::F32x4, 0, 0, mInstancedBuffer, 1 );

    mProgram = mRenderer.AddProgram( (const u8**)gVertexShader, (const u8**)gFragmentShader );
    mRenderer.UseProgram(mProgram);

    mat4* matrices = new mat4[gQuadCount*gQuadCount];
    f32 x(-1.0f);
    f32 y(-1.0f + 1.0f/(f32)gQuadCount);
    for( u32 row(0); row != gQuadCount; ++row )
    {
      x = -1.0f + 1.0f/(f32)gQuadCount;
      for( u32 column(0); column != gQuadCount; ++column )
      {
        matrices[row*gQuadCount + column] = ComputeTransform( vec3(x,y,0.0f ), vec3(1.0f/(f32)gQuadCount,1.0f/(f32)gQuadCount,1.0f), QUAT_UNIT );
        x += 2.0f/(f32)gQuadCount;
      }
      y += 2.0f/(f32)gQuadCount;
    }

    mShaderStorageBuffer = mRenderer.AddBuffer( sizeof( mat4 )*gQuadCount*gQuadCount, (void*)matrices );
    delete[] matrices;
    mRenderer.BindShaderStorageBuffer( mShaderStorageBuffer, 0 );

    mRenderer.SetClearColor( Dodo::vec4(1.0f,1.0f,1.0f,1.0f));
    mRenderer.SetBlendingMode( BLEND_ADD );
    mRenderer.SetBlendingFunction( BLENDING_SOURCE_ALPHA, BLENDING_ONE_MINUS_SOURCE_ALPHA, BLENDING_SOURCE_ALPHA, BLENDING_ONE_MINUS_SOURCE_ALPHA);
  }


  void Render()
  {
    mRenderer.ClearBuffers( Dodo::COLOR_BUFFER );
    mRenderer.SetupMeshVertexFormat(mQuad);
    mRenderer.SetupInstancedAttribute( VERTEX_ATTRIBUTE_4, mInstancedAttribute );
    mRenderer.DrawMeshInstanced( mQuad, gQuadCount*gQuadCount );

    ++gFrameCount;
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderer.SetViewport( 0, 0, width, height );
  }

  void OnMouseButton( Dodo::MouseButton button, f32 x, f32 y, bool pressed )
  {
    if( pressed )
    {
      vec4* instanceColor = (vec4*)mRenderer.MapBuffer( mInstancedBuffer, Dodo::BUFFER_MAP_WRITE );
      if(instanceColor)
      {
        for( u32 i(0); i != gQuadCount*gQuadCount; ++i )
        {
          instanceColor[i] = vec4( (rand()%255)/255.0f,(rand()%255)/255.0f,(rand()%255)/255.0f, 1.0f );
        }
        mRenderer.UnmapBuffer();
      }
    }
  }

private:

  MeshId mQuad;
  BufferId mShaderStorageBuffer;
  ProgramId mProgram;
  AttributeDescription mInstancedAttribute;
  Dodo::BufferId mInstancedBuffer;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

