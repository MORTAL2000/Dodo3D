
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
                                            "out vec3 lightDirection_tangentspace;\n"
                                            "out vec2 uv;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uView;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  uv = aTexCoord;\n"
                                            "  vec3 lightDirection_cameraspace = (uView * vec4(normalize(vec3(-1.0,1.0,0.0)),0.0)).xyz;\n"
                                            "  vec3 vertexNormal_cameraspace = (uModelView * vec4(0.0,0.0,1.0,0.0)).xyz;\n"
                                            "  vec3 vertexTangent_cameraspace = (uModelView * vec4(1.0,0.0,0.0,0.0)).xyz;\n"
                                            "  vec3 vertexBitangent_cameraspace = (uModelView * vec4(0.0,1.0,0.0,0.0)).xyz;\n"
                                            "  mat3 TBN = transpose(mat3(vertexTangent_cameraspace,vertexBitangent_cameraspace, vertexNormal_cameraspace));\n"
                                            "  lightDirection_tangentspace = TBN * lightDirection_cameraspace;\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                              "#version 440 core\n"
                                              "in vec2 uv;\n"
                                              "out vec3 color;\n"
                                              "uniform mat4 uModelView;\n"
                                              "in vec3 lightDirection_tangentspace;\n"
                                              "uniform sampler2D uNormalMap;\n"
                                              "uniform sampler2D uColorMap;\n"
                                              "void main(void)\n"
                                              "{\n"
                                              "  vec3 normal_tagentspace = normalize(texture2D( uNormalMap, uv ).rgb*2.0 - 1.0);\n"
                                              "  float diffuse = max(0.0,dot( normal_tagentspace,normalize(lightDirection_tangentspace) ) );\n"
                                              "  color = texture(uColorMap,uv).rgb * vec3(diffuse,diffuse,diffuse);\n"
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


class App : public Dodo::Application
{
public:
  App()
:Dodo::Application("Demo",500,500,4,4),
 mTxManager(1),
 mCamera(Dodo::vec3(0.0f,6.0f,8.0f), Dodo::vec2(0.0f,0.0f), 1.0f ),
 mAngle(0.0),
 mShader(),
 mQuad(),
 mMousePosition(0.0f,0.0f),
 mMouseButtonPressed(false)
  {}

  ~App()
  {}

  void Init()
  {
    mTxId0 = mTxManager.CreateTransform(Dodo::vec3(0.0f,0.0f,0.0f), Dodo::vec3(1.0f,1.0f,1.0f), Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), Dodo::DegreeToRadian(-90.0f)));
    mTxManager.Update();
    mProjection = Dodo::ComputeProjectionMatrix( Dodo::DegreeToRadian(75.0f),(f32)mWindowSize.x / (f32)mWindowSize.y,1.0f,500.0f );

    //Load resources
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    Dodo::Image normalImage("data/bric_normal.png");
    mNormalMap = mRenderManager.Add2DTexture( normalImage,true );
    Dodo::Image colorImage("data/bric_diffuse.png");
    mColorMap = mRenderManager.Add2DTexture( colorImage,true );
    mQuad = mRenderManager.CreateQuad(Dodo::uvec2(10u,10u), true, true );

    //Set GL state
    mRenderManager.SetCullFace( Dodo::CULL_NONE );
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( Dodo::vec4(0.1f,0.0f,0.65f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );
  }

  void Render()
  {
    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );

    //Draw mesh
    Dodo::mat4 modelMatrix;
    mTxManager.GetWorldTransform( mTxId0, &modelMatrix );
    mRenderManager.UseProgram( mShader );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uView"), mCamera.txInverse );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"),  modelMatrix * mCamera.txInverse );
    mRenderManager.Bind2DTexture( mColorMap, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uColorMap"), 0 );
    mRenderManager.Bind2DTexture( mNormalMap, 1 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uNormalMap"), 1 );
    mRenderManager.SetupMeshVertexFormat( mQuad );
    mRenderManager.DrawMesh( mQuad );
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
    mProjection = Dodo::ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
  }

  void OnKey( Dodo::Key key, bool pressed )
  {
    if( pressed )
    {
      switch( key )
      {
        case Dodo::KEY_UP:
        case 'w':
        {
          mCamera.Move( 0.0f, -1.0f );
          break;
        }
        case Dodo::KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, 1.0f );
          break;
        }
        case Dodo::KEY_LEFT:
        case 'a':
        {
          mCamera.Move( 1.0f, 0.0f );
          break;
        }
        case Dodo::KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -1.0f, 0.0f );
          break;
        }
        default:
          break;
      }
    }
  }

  void OnMouseButton( Dodo::MouseButton button, f32 x, f32 y, bool pressed )
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

      mCamera.Rotate( angleY,angleX );
      mMousePosition.x = x;
      mMousePosition.y = y;
    }
  }

private:

  Dodo::TxManager mTxManager;
  FreeCamera mCamera;
  Dodo::Id mTxId0;
  f32 mAngle;
  Dodo::ProgramId   mShader;
  Dodo::MeshId      mQuad;
  Dodo::TextureId  mColorMap;
  Dodo::TextureId  mNormalMap;


  Dodo::mat4 mProjection;
  Dodo::vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}
