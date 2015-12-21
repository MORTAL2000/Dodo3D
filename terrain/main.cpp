
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
                                     "uniform sampler2D uTexture;\n"
                                     "out vec2 uv;\n"
                                     "out vec3 normal;\n"
                                     "uniform mat4 uModelViewProjection;\n"
                                     "uniform mat4 uModel;\n"
                                     "uniform float uElevation;\n"
                                     "uniform uvec2 uHeightMapSize;\n"
                                     "uniform uvec2 uMapSize;\n"

                                     "const ivec3 off = ivec3(-1,0,1);\n"
                                     "void main(void)\n"
                                     "{\n"
                                     "  float elevation = textureOffset(uTexture, aTexCoord, ivec2(0,0)).x * uElevation;\n"
                                     "  vec3 position = aPosition + elevation*aNormal;\n"
                                     "  gl_Position = uModelViewProjection * vec4(position,1.0);\n"
                                     "  float right = textureOffset(uTexture, aTexCoord, off.zy).x * uElevation;\n"
                                     "  float up = textureOffset(uTexture, aTexCoord, off.yz).x * uElevation;\n"
                                     "  float left = textureOffset(uTexture, aTexCoord, off.xy).x * uElevation;\n"
                                     "  float down = textureOffset(uTexture, aTexCoord, off.yx).x * uElevation;\n"
                                     "  vec3 tangent = vec3(float(uMapSize.x)/float(uHeightMapSize.x),0.0,0.0);\n"
                                     "  vec3 binormal = vec3(0.0,float(uMapSize.y)/float(uHeightMapSize.y),0.0);\n"
                                     "  vec3 positionRight = ( aPosition  + tangent) + right*aNormal;\n"
                                     "  vec3 positionUp = ( aPosition + binormal)+ up*aNormal;\n"
                                     "  vec3 dx = normalize( positionRight - position);\n"
                                     "  vec3 dy = normalize( positionUp - position );\n"
                                     "  normal = normalize( (uModel * vec4(cross( dx, dy ),0.0)).xyz);\n"
                                     "  uv = aTexCoord;\n"
                                     "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                       "#version 440 core\n"
                                       "in vec2 uv;\n"
                                       "out vec3 color;\n"
                                       "in vec3 normal;\n"
                                       "uniform vec3 uLightDirection;\n"
                                       "uniform sampler2D uColorTexture;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "  vec3 lightDirection = normalize(uLightDirection);\n"
                                       "  float diffuse = max(0.0,dot( normal,lightDirection ) );\n"
                                       "  color = texture(uColorTexture, uv ).xyz * (diffuse+0.1);\n"
                                       "}\n"
};


const char* gVertexShaderSkybox[] = {
                                     "#version 440 core\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "layout (location = 1 ) in vec2 aTexCoord;\n"
                                     "uniform mat4 uViewProjectionInverse;\n"
                                     "out vec3 uv;\n"

                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = vec4(aPosition,1.0);\n"
                                     "  vec4 positionInWorldSpace = uViewProjectionInverse*gl_Position;\n"
                                     "  uv = positionInWorldSpace.xyz;\n"
                                     "}\n"
};
const char* gFragmentShaderSkybox[] = {
                                       "#version 440 core\n"
                                       "uniform samplerCube uTexture0;\n"
                                       "in vec3 uv;\n"
                                       "out vec4 color;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "  color =texture(uTexture0, uv );\n"
                                       "}\n"
};

struct Vertex
{
  Dodo::vec3 position;
  Dodo::vec2 uv;
  Dodo::vec3 normal;
};

}

struct LightAnimation
{
  void Run( f32 timeDelta )
  {
    f32 a = mCurrentTime + (timeDelta * 0.001f);
    if( a > mDuration )
      mCurrentTime = a - mDuration;
    else
      mCurrentTime = a;

    f32 t = mCurrentTime / mDuration;

    f32 angle = t * 6.28;
    Dodo::quat q = Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), angle );
    mLightDirection = Dodo::Rotate( Dodo::vec3(0.0f,0.0f,1.0f), q );
  }

  Dodo::vec3 mLightDirection;
  f32 mDuration;
  f32 mCurrentTime;
};

class App : public Dodo::Application
{
public:
  App()
 :Dodo::Application("Demo",500,500,4,4),
  mTxManager(0),
  mElevation(0.0),
  mShader(),
  mGrid(),
  mCameraPosition(0.0f,0.0f,0.0f),
  mCameraDirection(0.0f,0.0f,-1.0f),
  mCameraAngle(0.0f,0.0f,0.0f),
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

    //Create shaders
    mSkyboxShader = mRenderManager.AddProgram((const u8**)gVertexShaderSkybox, (const u8**)gFragmentShaderSkybox);
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);


    //Load resources
    Dodo::Image image("data/terrain-heightmap.bmp");
    mHeightMap = mRenderManager.Add2DTexture( image,false );
    mHeightmapSize = Dodo::uvec2(image.mWidth, image.mHeight);
    Dodo::Image colorImage("data/terrain-color.jpg");
    mColorMap = mRenderManager.Add2DTexture( colorImage,true );
    mMapSize = Dodo::uvec2(100u,100u);
    mElevation = mMapSize.x / 10.0f;
    mGrid = mRenderManager.CreateQuad(mMapSize, true, true, mHeightmapSize );

    Dodo::Image cubemapImages[6];
    cubemapImages[0].LoadFromFile( "data/sky/xpos.png", false);
    cubemapImages[1].LoadFromFile( "data/sky/xneg.png", false);
    cubemapImages[2].LoadFromFile( "data/sky/ypos.png", false);
    cubemapImages[3].LoadFromFile( "data/sky/yneg.png", false);
    cubemapImages[4].LoadFromFile( "data/sky/zpos.png", false);
    cubemapImages[5].LoadFromFile( "data/sky/zneg.png", false);
    mCubeMap = mRenderManager.AddCubeTexture(&cubemapImages[0]);
    mQuad = mRenderManager.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Set GL state
    mRenderManager.SetCullFace( Dodo::CULL_NONE );
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( Dodo::vec4(0.1f,0.0f,0.65f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );


    lightAnimation.mDuration = 10.0f;

  }

  void ComputeViewMatrix()
  {
    Dodo::quat cameraOrientation =  Dodo::QuaternionFromAxisAngle( Dodo::vec3(0.0f,1.0f,0.0f), mCameraAngle.x ) *
                                    Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), mCameraAngle.y );
    Dodo::ComputeInverse( Dodo::ComputeTransform(mCameraPosition, Dodo::VEC3_ONE,  cameraOrientation ), mView );
    mCameraDirection = Dodo::Rotate( Dodo::vec3(0.0f,0.0f,1.0f), cameraOrientation );
    mCameraDirection.Normalize();

    Dodo::mat4 mViewNoTranslation( mView );
    mViewNoTranslation.SetTranslation( Dodo::VEC3_ZERO );
    Dodo::ComputeInverse(mViewNoTranslation * mProjection, mSkyboxTransform );
  }

  void Render()
  {
    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );

    lightAnimation.Run( GetTimeDelta() );
    //Draw skybox
    mRenderManager.UseProgram( mSkyboxShader );
    mRenderManager.BindCubeTexture( mCubeMap, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mSkyboxShader,"uTexture0"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mSkyboxShader,"uViewProjectionInverse"), mSkyboxTransform );
    mRenderManager.DisableDepthWrite();
    mRenderManager.SetupMeshVertexFormat( mQuad );
    mRenderManager.DrawMesh( mQuad );
    mRenderManager.EnableDepthWrite();

    //Draw terrain
    Dodo::mat4 modelMatrix;
    mTxManager->GetWorldTransform( mTxId0, &modelMatrix );
    mRenderManager.UseProgram( mShader );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mView * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModel"),  modelMatrix  );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uElevation"), mElevation);
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uLightDirection"), lightAnimation.mLightDirection );
    mRenderManager.Bind2DTexture( mHeightMap, 0 );
    mRenderManager.Bind2DTexture( mColorMap, 1 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uHeightMapSize"), mHeightmapSize );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uMapSize"), mMapSize );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uTexture"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uColorTexture"), 1 );
    mRenderManager.SetupMeshVertexFormat( mGrid );
    mRenderManager.DrawMesh( mGrid );
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
  f32 mElevation;

  Dodo::ProgramId   mShader;
  Dodo::ProgramId   mSkyboxShader;
  Dodo::MeshId      mGrid;
  Dodo::uvec2       mMapSize;
  Dodo::TextureId  mHeightMap;
  Dodo::uvec2      mHeightmapSize;
  Dodo::TextureId  mColorMap;

  Dodo::MeshId     mQuad;
  Dodo::TextureId  mCubeMap;

  Dodo::mat4 mSkyboxTransform;
  Dodo::mat4 mProjection;
  Dodo::mat4 mView;
  Dodo::vec3 mCameraPosition;
  Dodo::vec3 mCameraDirection;
  Dodo::vec3 mCameraAngle;
  f32 mCameraVelocity;
  Dodo::vec2 mMousePosition;
  bool mMouseButtonPressed;

  LightAnimation lightAnimation;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

