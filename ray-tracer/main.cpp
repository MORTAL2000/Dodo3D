
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>
#include <render-manager.h>
#include <material.h>
#include <camera.h>
#include <cstdlib>
#include "ray-tracer.h"

using namespace Dodo;

namespace
{
const char* gVSFullScreen[] = {
                               "#version 440 core\n"
                               "out vec2 uv;\n"
                               "void main(void)\n"
                               "{\n"
                               "  uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);"
                               "  gl_Position = vec4(uv * vec2(2.0f, 2.0f) + vec2(-1.0f, -1.0f), 0.0f, 1.0f);\n"
                               "}\n"
};
const char* gFSFullScreen[] = {
                               "#version 440 core\n"
                               "in vec2 uv;\n"
                               "uniform sampler2D uTexture;\n"
                               "out vec4 color;\n"
                               "void main(void)\n"
                               "{\n"
                               "  color = texture( uTexture,uv );\n"
                               "}\n"
};

}



class App : public Application
{
public:

  App()
  :Application("Demo",600,600,4,4),
   mImageResolution(400,400),
   mTileSize(100,100),
   mRenderer(),
   mCamera( 70.0f, 0.2f, 5.0f, mImageResolution.x, mImageResolution.y ),
   mScene(),
   mMousePosition(0.0f,0.0f),
   mMouseButtonPressed(false)
  {}

  ~App()
  {
  }

  u32 GenerateMaterials()
  {
    RayTracer::Scene::Material m0;
    m0.albedo = vec3(0.8f,0.05f,0.05f);
    m0.roughness = 0.0f;
    m0.metalness = 1.0f;
    mScene.AddMaterial( m0 );

    m0.albedo = vec3(0.8f,0.8f,0.8f);
    m0.roughness = 0.2f;
    m0.metalness = 0.0f;
    mScene.AddMaterial( m0 );

    m0.albedo = vec3(0.05f,0.85f,0.05f);
    m0.roughness = 0.2f;
    m0.metalness = 0.0f;
    mScene.AddMaterial( m0 );

    m0.albedo = vec3(0.8f,0.85f,0.05f);
    m0.roughness = 0.9f;
    m0.metalness = 1.0f;
    mScene.AddMaterial( m0 );

    m0.albedo = vec3(0.8f,0.85f,0.8f);
    m0.roughness = 0.1f;
    m0.metalness = 1.0f;
    mScene.AddMaterial( m0 );

    return 4;
  }

  void GenerateScene( u32 sphereCount, u32 materialCount, const vec3& extents )
  {
    unsigned short seed[3] = {time(0),time(0),time(0)};
    seed48(&seed[0]);

    //Ground
    RayTracer::Scene::Sphere ground;
    ground.center = vec3(0.0f,-100000.0f,0.0f);
    ground.radius = 100000.0f - 1.0f;
    ground.material = 4;
    mScene.AddSphere( ground );

    std::vector<RayTracer::Scene::Sphere> spheres(sphereCount);
    for( u32 i(0); i<sphereCount; ++i )
    {
      bool ok(false);
      f32 radius;
      vec3 center;
      do
      {
        ok = true;
        radius = (drand48()+0.3f) * 1.5f;
        center = vec3( (2.0f * drand48() - 1.0f) * extents.x, radius - 1.0001f, (2.0f * drand48() - 1.0f ) * extents.z);
        for( u32 j(0); j<i; ++j )
        {
          f32 distance = Lenght( center - spheres[j].center );
          if( distance < radius + spheres[j].radius )
          {
            ok = false;
            break;
          }
        }

      }while( !ok );

      spheres[i].radius = radius;
      spheres[i].center = center;
      spheres[i].material = u32( drand48() * materialCount );
      mScene.AddSphere( spheres[i] );
    }
  }

  void Init()
  {
    u32 bufferSize(mImageResolution.x*mImageResolution.y*3);
    mBuffer = mRenderManager.AddBuffer(bufferSize, 0 );
    mTexture = mRenderManager.Add2DTexture(mImageResolution.x, mImageResolution.y, FORMAT_RGB8, false);
    mShaderFullScreen = mRenderManager.AddProgram((const u8**)gVSFullScreen, (const u8**)gFSFullScreen);
    GenerateScene( 10, GenerateMaterials(), vec3(10.0f,0.0f,10.0f) );

    mRenderer.Initialize( mImageResolution, mTileSize, &mCamera, &mScene );
  }


  void Render()
  {
    static f32 time = 0.0f;
    static u32 frame = 0;

    if( time > 1000.0f )
    {
       std::cout<<frame<<std::endl;
       time = 0.0f;
       frame = 0u;
    }
    time += GetTimeDelta();
    frame++;

    mRenderer.Render();

    //Update buffer
    u32 bufferSize(mImageResolution.x * mImageResolution.y * 3);
    mRenderManager.UpdateBuffer( mBuffer, bufferSize, mRenderer.GetImage() );

    //Write data to texture
    mRenderManager.Update2DTextureFromBuffer(mTexture, mImageResolution.x, mImageResolution.y, FORMAT_RGB8, mBuffer );

    mRenderManager.ClearBuffers( COLOR_BUFFER | DEPTH_BUFFER );
    mRenderManager.UseProgram( mShaderFullScreen );
    mRenderManager.Bind2DTexture( mTexture, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uTexture0"), 0 );
    mRenderManager.DrawCall( 3 );
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
  }

  void OnKey( Key key, bool pressed )
  {
    if( pressed )
    {
      switch( key )
      {
        case KEY_UP:
        case 'w':
        {
          mCamera.MoveCamera( 0.0f, -1.0f);
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.MoveCamera( 0.0f, 1.0f );
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.MoveCamera( 1.0f, 0.0f );
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.MoveCamera( -1.0f, 0.0f );
          break;
        }
        default:
          break;
      }
    }
  }

  void OnMouseButton( MouseButton button, f32 x, f32 y, bool pressed )
  {
    mMouseButtonPressed = pressed;
    mMousePosition.x = x;
    mMousePosition.y = y;
  }

  void OnMouseMove( f32 x, f32 y )
  {

    if( mMouseButtonPressed )
    {
      f32 angleY = (x - mMousePosition.x) * 0.01f;
      f32 angleX = (y - mMousePosition.y) * 0.01f;

      mCamera.RotateCamera( angleX, angleY );

      mMousePosition.x = x;
      mMousePosition.y = y;
    }
  }

private:

  uvec2 mImageResolution;
  uvec2 mTileSize;
  BufferId mBuffer;             ///<Used to upload data to the texture
  TextureId mTexture;           ///<Texture with the image generated by the renderer
  ProgramId mShaderFullScreen;  ///<Fullscreen quad to draw the texture

  RayTracer::Renderer mRenderer;
  RayTracer::Camera mCamera;
  RayTracer::Scene mScene;

  vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}



