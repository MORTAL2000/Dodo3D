
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <scene-graph.h>
#include <types.h>
#include <render-manager.h>

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


}


class App : public Dodo::Application
{
public:
  App()
 :Dodo::Application("Demo",500,500,4,4),
  mTxManager(0),
  mAngle(0.0),
  mShader(),
  mQuad(),
  mCameraPosition(0.0f,6.0f,8.0f),
  mCameraDirection(0.0f,0.0f,-1.0f),
  mCameraAngle(0.0f,-0.8f,0.0f),
  mCameraVelocity(1.0f),
  mMousePosition(0.0f,0.0f),
  mMouseButtonPressed(false)
  {}

  ~App()
  {}

  void Init()
  {
    mTxManager = new Dodo::TxManager( 10 );
    mTxId0 = mTxManager->CreateTransform(Dodo::vec3(0.0f,0.0f,0.0f), Dodo::vec3(1.0f,1.0f,1.0f), Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), Dodo::DegreeToRadian(-90.0f)));
    mTxManager->Update();
    mProjection = Dodo::ComputeProjectionMatrix( Dodo::DegreeToRadian(75.0f),(f32)mWindowSize.x / (f32)mWindowSize.y,1.0f,500.0f );
    ComputeViewMatrix();
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);


    //Load resources
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

  void ComputeViewMatrix()
  {
    Dodo::quat cameraOrientation =  Dodo::QuaternionFromAxisAngle( Dodo::vec3(0.0f,1.0f,0.0f), mCameraAngle.x ) *
                                    Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), mCameraAngle.y );
    Dodo::ComputeInverse( Dodo::ComputeTransform(mCameraPosition, Dodo::VEC3_ONE,  cameraOrientation ), mView );
    mCameraDirection = Dodo::Rotate( Dodo::vec3(0.0f,0.0f,1.0f), cameraOrientation );
    mCameraDirection.Normalize();
  }

  void Render()
  {
    mAngle += 0.5f;


    mTxManager->UpdateTransform(mTxId0,Dodo::TxComponent(Dodo::vec3(0.0f,0.0f,0.0f), Dodo::vec3(1.0f,1.0f,1.0f), Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), Dodo::DegreeToRadian(-90.0f))*
                                                                                        Dodo::QuaternionFromAxisAngle( Dodo::vec3(0.0f,0.0f,1.0f), Dodo::DegreeToRadian(mAngle))));
    mTxManager->Update();

    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );

    //Draw mesh
    Dodo::mat4 modelMatrix;
    mTxManager->GetWorldTransform( mTxId0, &modelMatrix );
    mRenderManager.UseProgram( mShader );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mView * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uView"), mView );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"),  modelMatrix * mView );
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
          mCameraPosition = mCameraPosition - mCameraVelocity * mCameraDirection;
          ComputeViewMatrix();

          break;
        }
        case Dodo::KEY_DOWN:
        case 's':
        {
          mCameraPosition = mCameraPosition + mCameraVelocity * mCameraDirection;
          ComputeViewMatrix();
          break;
        }
        case Dodo::KEY_LEFT:
        case 'a':
        {
          mCameraPosition = mCameraPosition + mCameraVelocity * Dodo::Cross( mCameraDirection, Dodo::vec3(0.0f,1.0f,0.0f));
          ComputeViewMatrix();
          break;
        }
        case Dodo::KEY_RIGHT:
        case 'd':
        {
          mCameraPosition = mCameraPosition - mCameraVelocity * Dodo::Cross( mCameraDirection, Dodo::vec3(0.0f,1.0f,0.0f));
          ComputeViewMatrix();
          break;
        }
        case 'z':
        {
          static bool wired = false;
          wired = !wired;
          mRenderManager.Wired(wired);
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
      mCameraAngle.x -= (x - mMousePosition.x) * 0.01f;

      f32 newAngleY = mCameraAngle.y - ((y - mMousePosition.y) * 0.01f);
      if( newAngleY < M_PI_2 && newAngleY > -M_PI_2 )
      {
        mCameraAngle.y = newAngleY;
      }

      mMousePosition.x = x;
      mMousePosition.y = y;
      ComputeViewMatrix();
    }
  }

private:

  Dodo::TxManager* mTxManager;
  Dodo::Id mTxId0;
  f32 mAngle;
  Dodo::ProgramId   mShader;
  Dodo::MeshId      mQuad;
  Dodo::TextureId  mColorMap;
  Dodo::TextureId  mNormalMap;


  Dodo::mat4 mProjection;
  Dodo::mat4 mView;
  Dodo::vec3 mCameraPosition;
  Dodo::vec3 mCameraDirection;
  Dodo::vec3 mCameraAngle;
  f32 mCameraVelocity;
  Dodo::vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}
