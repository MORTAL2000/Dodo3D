
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
 mTxManager(1),
 mAngle(0.0),
 mCamera( vec3(0.0f,0.0f,50.0f), vec2(0.0f,0.0f), 1.0f ),
 mMousePosition(0.0f,0.0f),
 mMouseButtonPressed(false)
{}

  ~App()
  {}

  void Init()
  {
    mTxId0 = mTxManager.CreateTransform();
    mProjection = ComputeProjectionMatrix( DegreeToRadian(75.0f),(f32)mWindowSize.x / (f32)mWindowSize.y,1.0f,500.0f );
    ComputeSkyBoxTransform();

    //Create a texture
    Image image("data/jeep.jpg");
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
    mRenderManager.SetCullFace( CULL_BACK );
    mRenderManager.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( vec4(0.1f,0.0f,0.65f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );

    mFbo =  mRenderManager.AddFrameBuffer();
    mColorAttachment = mRenderManager.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );

    //Create cube map
    Image cubemapImages[6];
    cubemapImages[0].LoadFromFile( "data/sky/xpos.png", false);
    cubemapImages[1].LoadFromFile( "data/sky/xneg.png", false);
    cubemapImages[2].LoadFromFile( "data/sky/ypos.png", false);
    cubemapImages[3].LoadFromFile( "data/sky/yneg.png", false);
    cubemapImages[4].LoadFromFile( "data/sky/zpos.png", false);
    cubemapImages[5].LoadFromFile( "data/sky/zneg.png", false);
    mCubeMap = mRenderManager.AddCubeTexture(&cubemapImages[0]);
  }

  void Render()
  {
    mAngle -=  GetTimeDelta() * 0.001f;
    quat newOrientation(vec3(0.0f,1.0f,0.0f), mAngle);
    mTxManager.UpdateTransform( mTxId0, TxComponent( vec3(0.0f,0.0f,0.0f), VEC3_ONE, newOrientation ) );
    mTxManager.Update();

    mRenderManager.BindFrameBuffer( mFbo );
    mRenderManager.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );

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
    mat4 modelMatrix;
    mTxManager.GetWorldTransform( mTxId0, &modelMatrix );
    mRenderManager.UseProgram( mShader );
    mRenderManager.Bind2DTexture( mColorTexture, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uTexture0"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"), modelMatrix * mCamera.txInverse );
    mRenderManager.SetupMeshVertexFormat( mMesh );
    mRenderManager.DrawMesh( mMesh );

    //Draw bounding box
    mRenderManager.UseProgram( mAABBShader );
    Mesh mesh = mRenderManager.GetMesh(mMesh);
    mat4 aabbModel = ComputeTransform( mesh.mAABB.mCenter, 2.0f * mesh.mAABB.mExtents, QUAT_UNIT );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mAABBShader,"uModelViewProjection"), (aabbModel * modelMatrix) * mCamera.txInverse * mProjection );
    mRenderManager.Wired( true );
    mRenderManager.SetCullFace( CULL_NONE );
    mRenderManager.SetupMeshVertexFormat( mBox );
    mRenderManager.DrawMesh( mBox );
    mRenderManager.Wired( false );
    mRenderManager.SetCullFace( CULL_BACK );

    //Draw quad with offscreen color buffer
    mRenderManager.BindFrameBuffer( 0 );
    mRenderManager.ClearBuffers( COLOR_BUFFER | DEPTH_BUFFER );
    mRenderManager.UseProgram( mQuadShader );
    mRenderManager.Bind2DTexture( mColorAttachment, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mQuadShader,"uTexture0"), 0 );
    mRenderManager.SetupMeshVertexFormat( mQuad );
    mRenderManager.DrawMesh( mQuad );
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
    mProjection = ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
    ComputeInverse( mProjection, mProjectionI );
    //Recreate framebuffer attachments to match new size
    mRenderManager.RemoveTexture(mColorAttachment);
    mRenderManager.RemoveTexture(mDepthStencilAttachment);
    mColorAttachment = mRenderManager.Add2DTexture( Image( width, height, 0, FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Image( width, height, 0, FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );
  }

  void ComputeSkyBoxTransform()
  {
    mat4 mViewNoTranslation( mCamera.txInverse );
    mViewNoTranslation.SetTranslation( VEC3_ZERO );
    ComputeInverse(mViewNoTranslation * mProjection, mSkyBoxTransform );
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
          mCamera.Move( 0.0f, -1.0f );
          ComputeSkyBoxTransform();
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, 1.0f );
          ComputeSkyBoxTransform();
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.Move( 1.0f, 0.0f );
          ComputeSkyBoxTransform();
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -1.0f, 0.0f );
          ComputeSkyBoxTransform();
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

      mCamera.Rotate( angleY,angleX );
      ComputeSkyBoxTransform();

      mMousePosition.x = x;
      mMousePosition.y = y;
    }
  }

private:

  TxManager mTxManager;
  TxId mTxId0;
  f32 mAngle;

  MeshId      mBox;
  MeshId      mMesh;
  TextureId   mColorTexture;
  TextureId   mCubeMap;
  ProgramId   mShader;

  MeshId      mQuad;
  ProgramId   mQuadShader;
  ProgramId   mAABBShader;
  ProgramId   mSkyboxShader;

  FBOId mFbo;
  TextureId mColorAttachment;
  TextureId mDepthStencilAttachment;

  FreeCamera mCamera;
  mat4 mProjection;
  mat4 mProjectionI;
  mat4 mSkyBoxTransform;
  vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

