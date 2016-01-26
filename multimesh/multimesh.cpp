
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <scene-graph.h>
#include <types.h>
#include <render-manager.h>

using namespace Dodo;

namespace
{

const char* gVertexShaderSourceDiffuse[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 1 ) in vec2 aTexCoord;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"

                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"

                                            "out vec3 normal_cameraspace;"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  normal_cameraspace = (uModelView * vec4(aNormal,0.0)).xyz;\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                              "#version 440 core\n"
                                              "in vec3 lightDirection_cameraspace;\n"
                                              "in vec3 normal_cameraspace;"
                                              "out vec4 color;\n"
                                              "void main(void)\n"
                                              "{\n"
                                              "  color = vec4( normal_cameraspace, 1.0);\n"
                                              "}\n"
};

struct FreeCamera
{
  FreeCamera( const vec3& position, const vec2& angle, f32 velocity )
  :mPosition(position),
   mAngle(angle),
   mVelocity(velocity)
  {
    UpdateTx();
  }

  void Move( f32 xAmount, f32 zAmount )
  {
    mPosition = mPosition + ( zAmount * mVelocity * mForward ) + ( xAmount * mVelocity * mRight );
    UpdateTx();
  }

  void Rotate( f32 angleY, f32 angleZ )
  {
    mAngle.x =  mAngle.x - angleY;
    angleY = mAngle.y - angleZ;
    if( angleY < M_PI_2 && angleY > -M_PI_2 )
    {
      mAngle.y = angleY;
    }

    UpdateTx();
  }

  void UpdateTx()
  {
    quat orientation =  QuaternionFromAxisAngle( vec3(0.0f,1.0f,0.0f), mAngle.x ) *
        QuaternionFromAxisAngle( vec3(1.0f,0.0f,0.0f), mAngle.y );

    mForward = Dodo::Rotate( vec3(0.0f,0.0f,1.0f), orientation );
    mRight = Cross( mForward, vec3(0.0f,1.0f,0.0f) );

    tx = ComputeTransform( mPosition, VEC3_ONE, orientation );
    ComputeInverse( tx, txInverse );
  }

  mat4 tx;
  mat4 txInverse;

  vec3 mPosition;
  vec3 mForward;
  vec3 mRight;
  vec2 mAngle;
  f32  mVelocity; //Units per second
};

}

class App : public Application
{
public:

  App()
  :Application("Demo",500,500,4,4),
   mMesh(),
   mShader(),
   mCamera( vec3(0.0f,1.0f,0.0f), vec2(0.0f,0.0f), 500.0f ),
   mMousePosition(0.0f,0.0f),
   mMouseButtonPressed(false)
  {}

  ~App()
  {}

  void Init()
  {
    //Create a shader
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);

    //Load meshes
    mMesh = mRenderManager.AddMultiMesh( "data/sponza.obj" );

    //Set GL state
    mRenderManager.SetCullFace( CULL_NONE );
    mRenderManager.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( vec4(0.1f,0.0f,0.65f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );
  }

  void Render()
  {
    mRenderManager.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );

    //Draw model
    mRenderManager.UseProgram( mShader );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"), mCamera.txInverse );
    mRenderManager.DrawMultiMesh( mMesh );
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
    mProjection = ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,1.0f,2000.0f );
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
          mCamera.Move( 0.0f, -GetTimeDelta()*0.001f);
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, GetTimeDelta()*0.001f );
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.Move( GetTimeDelta()*0.001f, 0.0f );
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -GetTimeDelta()*0.001f, 0.0f );
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
      f32 angleZ = (y - mMousePosition.y) * 0.01f;

      mCamera.Rotate( angleY, angleZ );

      mMousePosition.x = x;
      mMousePosition.y = y;
    }
  }

private:

  MultiMeshId mMesh;
  ProgramId   mShader;
  FreeCamera mCamera;
  mat4 mProjection;
  vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

