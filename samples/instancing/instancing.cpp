
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>
#include <render-manager.h>
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


class App : public Dodo::Application
{
public:
  App()
:Dodo::Application("Demo",500,500,4,4)
  {}

  ~App()
  {
    DODO_LOG("%u", gFrameCount );
  }

  void Init()
  {
    mQuad = mRenderManager.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Instanced attribute
    vec4* instanceColor = new vec4[gQuadCount*gQuadCount];
    for( u32 i(0); i != gQuadCount*gQuadCount; ++i )
    {
      instanceColor[i] = vec4( (rand()%255)/255.0f,(rand()%255)/255.0f,(rand()%255)/255.0f, 1.0f );
    }
    mInstancedBuffer = mRenderManager.AddBuffer( sizeof(vec4)*gQuadCount*gQuadCount, instanceColor );
    delete[] instanceColor;
    mInstancedAttribute = AttributeDescription( Dodo::F32x4, 0, 0, mInstancedBuffer, 1 );

    mProgram = mRenderManager.AddProgram( (const u8**)gVertexShader, (const u8**)gFragmentShader );
    mRenderManager.UseProgram(mProgram);

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

    mShaderStorageBuffer = mRenderManager.AddBuffer( sizeof( mat4 )*gQuadCount*gQuadCount, (void*)matrices );
    delete[] matrices;
    mRenderManager.BindShaderStorageBuffer( mShaderStorageBuffer, 0 );

    mRenderManager.SetClearColor( Dodo::vec4(1.0f,1.0f,1.0f,1.0f));
    mRenderManager.SetBlendingMode( BLEND_ADD );
    mRenderManager.SetBlendingFunction( BLENDING_SOURCE_ALPHA, BLENDING_ONE_MINUS_SOURCE_ALPHA, BLENDING_SOURCE_ALPHA, BLENDING_ONE_MINUS_SOURCE_ALPHA);
  }


  void Render()
  {
    mRenderManager.ClearBuffers( Dodo::COLOR_BUFFER );
    mRenderManager.SetupMeshVertexFormat(mQuad);
    mRenderManager.SetupInstancedAttribute( VERTEX_ATTRIBUTE_4, mInstancedAttribute );
    mRenderManager.DrawMeshInstanced( mQuad, gQuadCount*gQuadCount );

    ++gFrameCount;
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
  }

  void OnMouseButton( Dodo::MouseButton button, f32 x, f32 y, bool pressed )
  {
    if( pressed )
    {
      vec4* instanceColor = (vec4*)mRenderManager.MapBuffer( mInstancedBuffer, Dodo::BUFFER_MAP_WRITE );
      if(instanceColor)
      {
        for( u32 i(0); i != gQuadCount*gQuadCount; ++i )
        {
          instanceColor[i] = vec4( (rand()%255)/255.0f,(rand()%255)/255.0f,(rand()%255)/255.0f, 1.0f );
        }
        mRenderManager.UnmapBuffer();
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

