
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
                                     "out vec3 normal;\n"
                                     "out vec3 lightDirection;\n"
                                     "out vec2 uv;\n"
                                     "uniform mat4 uModelView;\n"
                                     "uniform mat4 uModelViewProjection;\n"
                                     "out vec3 viewVector;\n"
                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                     "  normal = (uModelView * vec4(aNormal,0.0)).xyz;\n"
                                     "  lightDirection = normalize((uModelView * vec4( 1.0,0.0,0.0,0.0)).xyz);\n"
                                     "  viewVector = -normalize( (uModelView * vec4(aPosition,1.0)).xyz);\n"
                                     "  uv = aTexCoord;\n"
                                     "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                       "#version 440 core\n"
                                       "uniform sampler2D uTexture0;\n"
                                       "uniform samplerCube uCubeMap;\n"
                                       "in vec3 normal;\n"
                                       "in vec3 lightDirection;\n"
                                       "in vec2 uv;\n"
                                       "in vec3 viewVector;\n"
                                       "out vec3 color;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "  float diffuse = max(0.0,dot( normal,lightDirection ) );\n"
                                       "  float ambient = 0.2;\n"
                                       "  vec3 reflection = normalize( -reflect( viewVector, normal));\n"
                                       " float specular = pow(max(dot(reflection,lightDirection),0.0),100.0);\n"
                                       "  color = (diffuse + ambient) * texture(uTexture0, uv ).rgb + vec3(specular,specular,specular);\n"
                                       "}\n"
};


const char* gVertexShaderSourceTexture[] = {
                                     "#version 440 core\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "layout (location = 1 ) in vec2 aTexCoord;\n"
                                     "out vec2 uv;\n"
                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = vec4(aPosition,1.0);\n"
                                     "  uv = aTexCoord;\n"
                                     "}\n"
};
const char* gFragmentShaderSourceTexture[] = {
                                       "#version 440 core\n"
                                       "uniform sampler2D uTexture0;\n"
                                       "in vec2 uv;\n"
                                       "out vec4 color;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "  color = texture(uTexture0, uv );\n"
                                       "}\n"
};


const char* gVertexShaderSourceAABB[] = {
                                     "#version 440 core\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "uniform mat4 uModelViewProjection;\n"
                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                     "}\n"
};
const char* gFragmentShaderSourceAABB[] = {
                                       "#version 440 core\n"
                                       "out vec3 color;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "  color = vec3(0.0,1.0,0.0);\n"
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

}

struct SqrtTask : public Dodo::ITask
{
  void Run()
  {
    for( size_t i(0); i< mCount; ++i )
    {
      mOut[i] = sqrtf( mIn[i] );
    }
  }

  float*  mIn;
  float*  mOut;
  size_t  mCount;
};


class App : public Dodo::Application
{
public:
  App()
 :Dodo::Application("Demo",500,500,4,4),
  mTxManager(0),
  mAngle(0.0),
  mCameraPosition(0.0f,0.0f,50.0f),
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
    mTxId0 = mTxManager->CreateTransform();
    mProjection = Dodo::ComputeProjectionMatrix( Dodo::DegreeToRadian(75.0f),(f32)mWindowSize.x / (f32)mWindowSize.y,1.0f,500.0f );

    //Create a texture
    Dodo::Image image("data/jeep.jpg");
    mColorTexture = mRenderManager.Add2DTexture( image );

    //Create a shader
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    mQuadShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceTexture, (const u8**)gFragmentShaderSourceTexture);
    mAABBShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceAABB, (const u8**)gFragmentShaderSourceAABB);
    mSkyboxShader = mRenderManager.AddProgram((const u8**)gVertexShaderSkybox, (const u8**)gFragmentShaderSkybox);

    //Load meshes
    mMesh = mRenderManager.AddMesh( "data/jeep.ms3d" );
    mBox = mRenderManager.AddMesh( "data/box.obj" );
    mQuad = mRenderManager.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Set GL state
    mRenderManager.SetCullFace( Dodo::CULL_BACK );
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( Dodo::vec4(0.1f,0.0f,0.65f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );

    mFbo =  mRenderManager.AddFrameBuffer();
    mColorAttachment = mRenderManager.Add2DTexture( Dodo::Image( mWindowSize.x, mWindowSize.y, 0, Dodo::FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Dodo::Image( mWindowSize.x, mWindowSize.y, 0, Dodo::FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );

    //Create cube map
    Dodo::Image cubemapImages[6];
    cubemapImages[0].LoadFromFile( "data/sky/xpos.png", false);
    cubemapImages[1].LoadFromFile( "data/sky/xneg.png", false);
    cubemapImages[2].LoadFromFile( "data/sky/ypos.png", false);
    cubemapImages[3].LoadFromFile( "data/sky/yneg.png", false);
    cubemapImages[4].LoadFromFile( "data/sky/zpos.png", false);
    cubemapImages[5].LoadFromFile( "data/sky/zneg.png", false);
    mCubeMap = mRenderManager.AddCubeTexture(&cubemapImages[0]);


    ComputeViewMatrix();
  }

  void ComputeViewMatrix()
  {
    Dodo::quat cameraOrientation =  Dodo::QuaternionFromAxisAngle( Dodo::vec3(0.0f,1.0f,0.0f), mCameraAngle.x ) *
                                    Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), mCameraAngle.y );

    mCameraDirection = Dodo::Rotate( Dodo::vec3(0.0f,0.0f,1.0f), cameraOrientation );
    mCameraDirection.Normalize();

    Dodo::ComputeInverse( Dodo::ComputeTransform(mCameraPosition, Dodo::VEC3_ONE,  cameraOrientation ), mView );

    Dodo::mat4 mViewNoTranslation( mView );
    mViewNoTranslation.SetTranslation( Dodo::VEC3_ZERO );
    Dodo::ComputeInverse(mViewNoTranslation * mProjection, mSkyBoxTransform );
  }

  void Render()
  {
    mAngle -=  GetTimeDelta() * 0.001f;
    Dodo::quat newOrientation(Dodo::vec3(0.0f,1.0f,0.0f), mAngle);
    mTxManager->UpdateTransform( mTxId0, Dodo::TxComponent( Dodo::vec3(0.0f,0.0f,0.0f), Dodo::VEC3_ONE, newOrientation ) );
    mTxManager->Update();

    mRenderManager.BindFrameBuffer( mFbo );
    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );

    //Draw skybox
    mRenderManager.UseProgram( mSkyboxShader );
    mRenderManager.BindCubeTexture( mCubeMap, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mSkyboxShader,"uTexture0"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mSkyboxShader,"uViewProjectionInverse"), mSkyBoxTransform );
    mRenderManager.DisableDepthWrite();
    mRenderManager.SetupMeshVertexFormat( mQuad );
    mRenderManager.DrawMesh( mQuad );
    mRenderManager.EnableDepthWrite();

    //Draw model
    Dodo::mat4 modelMatrix;
    mTxManager->GetWorldTransform( mTxId0, &modelMatrix );
    mRenderManager.UseProgram( mShader );
    mRenderManager.Bind2DTexture( mColorTexture, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uTexture0"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mView * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"), modelMatrix * mView );
    mRenderManager.SetupMeshVertexFormat( mMesh );
    mRenderManager.DrawMesh( mMesh );

    //Draw bounding box
    mRenderManager.UseProgram( mAABBShader );
    Dodo::Mesh mesh = mRenderManager.GetMesh(mMesh);
    Dodo::mat4 aabbModel = Dodo::ComputeTransform( mesh.mAABB.mCenter, 2.0f * mesh.mAABB.mExtents, Dodo::QUAT_UNIT );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mAABBShader,"uModelViewProjection"), (aabbModel * modelMatrix) * mView * mProjection );
    mRenderManager.Wired( true );
    mRenderManager.SetCullFace( Dodo::CULL_NONE );
    mRenderManager.SetupMeshVertexFormat( mBox );
    mRenderManager.DrawMesh( mBox );
    mRenderManager.Wired( false );
    mRenderManager.SetCullFace( Dodo::CULL_BACK );

    //Draw quad with offscreen color buffer
    mRenderManager.BindFrameBuffer( 0 );
    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );
    mRenderManager.UseProgram( mQuadShader );
    mRenderManager.Bind2DTexture( mColorAttachment, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mQuadShader,"uTexture0"), 0 );
    mRenderManager.SetupMeshVertexFormat( mQuad );
    mRenderManager.DrawMesh( mQuad );
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
    mProjection = Dodo::ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
    Dodo::ComputeInverse( mProjection, mProjectionI );
    //Recreate framebuffer attachments to match new size
    mRenderManager.RemoveTexture(mColorAttachment);
    mRenderManager.RemoveTexture(mDepthStencilAttachment);
    mColorAttachment = mRenderManager.Add2DTexture( Dodo::Image( width, height, 0, Dodo::FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Dodo::Image( width, height, 0, Dodo::FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );
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

  Dodo::MeshId      mBox;
  Dodo::MeshId      mMesh;
  Dodo::TextureId   mColorTexture;
  Dodo::TextureId   mCubeMap;
  Dodo::ProgramId   mShader;

  Dodo::MeshId      mQuad;
  Dodo::ProgramId   mQuadShader;
  Dodo::ProgramId   mAABBShader;
  Dodo::ProgramId   mSkyboxShader;

  Dodo::FBOId mFbo;
  Dodo::TextureId mColorAttachment;
  Dodo::TextureId mDepthStencilAttachment;

  Dodo::mat4 mProjection;
  Dodo::mat4 mProjectionI;
  Dodo::mat4 mView;
  Dodo::mat4 mSkyBoxTransform;
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

