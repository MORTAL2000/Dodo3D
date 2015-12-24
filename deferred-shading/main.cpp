
#include <application.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>

using namespace Dodo;

#define LIGHT_COUNT 5
#define OBJECT_COUNT 5

namespace
{

const char* gVertexShaderGBuffer[] = {
                                     "#version 440 core\n"
                                     "layout (std140, binding=0) uniform Matrices\n"
                                     "{\n "
                                     "  mat4 uViewMatrix;\n"
                                     "  mat4 uProjectionMatrix;\n"
                                     "  mat4 uViewProjectionMatrix;\n"
                                     "};\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "layout (location = 1 ) in vec2 aTexCoord;\n"
                                     "layout (location = 2 ) in vec3 aNormal;\n"
                                     "uniform mat4 uModelMatrix;\n"
                                     "out vec3 normal;\n"
                                     "out vec2 uv;\n"

                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = (uViewProjectionMatrix*uModelMatrix) * vec4(aPosition,1.0);\n"
                                     "  normal = (uViewMatrix*uModelMatrix * vec4(aNormal,0.0)).xyz;\n"
                                     "  uv = aTexCoord;\n"
                                     "}\n"
};
const char* gFragmentShaderGBuffer[] = {
                                       "#version 440 core\n"
                                       "uniform sampler2D uTexture0;\n"
                                       "in vec3 normal;\n"
                                       "in vec2 uv;\n"
                                       "layout (location = 0) out vec4 color;\n"
                                       "layout (location = 1) out vec4 outNormal;\n"

                                       "void main(void)\n"
                                       "{\n"
                                       "  color = texture(uTexture0, uv );\n"
                                       "  outNormal = vec4( normal, 1.0 );\n"
                                       "}\n"
};

const char* gVertexShaderColor[] = {
                                     "#version 440 core\n"
                                     "layout (std140, binding=0) uniform Matrices\n"
                                     "{\n "
                                     "  mat4 uViewMatrix;\n"
                                     "  mat4 uProjectionMatrix;\n"
                                     "  mat4 uViewProjectionMatrix;\n"
                                     "};\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "uniform vec3 uPosition;\n"
                                     "uniform float uScale;\n"
                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = uViewProjectionMatrix * vec4(aPosition*uScale+uPosition,1.0);\n"
                                     "}\n"
};
const char* gFragmentShaderColor[] = {
                                       "#version 440 core\n"
                                       "uniform vec3 uColor;\n"
                                       "uniform sampler2D uTexture0;\n"  //Depth
                                       "uniform vec2 uTextureSize;\n"
                                       "out vec3 color;\n"

                                       "void main(void)\n"
                                       "{\n"
                                       "  vec2 uv = gl_FragCoord.xy/uTextureSize;\n"
                                       "  if (gl_FragCoord.z > texture(uTexture0,uv).x ) discard;\n"
                                       "  color = uColor;\n"
                                       "}\n"
};

const char* gVertexShaderPointLight[] = {
                                     "#version 440 core\n"
                                     "layout (std140, binding=0) uniform Matrices\n"
                                     "{\n "
                                     "  mat4 uViewMatrix;\n"
                                     "  mat4 uProjectionMatrix;\n"
                                     "  mat4 uViewProjectionMatrix;\n"
                                     "};\n"
                                     "layout (location = 0 ) in vec3 aPosition;\n"
                                     "uniform vec3 uLightPosition;\n"
                                     "uniform float uRadius;\n"

                                     "void main(void)\n"
                                     "{\n"
                                     "  gl_Position = uViewProjectionMatrix * vec4( aPosition*uRadius+uLightPosition, 1.0 );\n"
                                     "}\n"
};
const char* gFragmentShaderPointLight[] = {
                                       "#version 440 core\n"
                                       "uniform sampler2D uTexture0;\n" //Diffuse
                                       "uniform sampler2D uTexture1;\n" //Normal
                                       "uniform sampler2D uTexture2;\n"  //Depth
                                       "uniform vec2 uTextureSize;\n"
                                       "uniform mat4 uProjectionInverse;\n"
                                       "uniform vec3 uLightPositionViewSpace;\n"
                                       "uniform vec3 uLightColor;\n"
                                       "uniform float uRadius;\n"
                                       "out vec4 color;\n"

                                       "vec3 ViewSpacePositionFromDepth(vec2 uv, float depth)\n"
                                       "{\n"
                                       "  vec3 clipSpacePosition = vec3(uv, depth) * 2.0 - vec3(1.0);\n"
                                       "  vec4 viewSpacePosition = uProjectionInverse * vec4(clipSpacePosition,1.0);\n"
                                       "  return(viewSpacePosition.xyz / viewSpacePosition.w);\n"
                                       "}\n"

                                       "void main(void)\n"
                                       "{\n"
                                       "  vec2 uv = gl_FragCoord.xy/uTextureSize;\n"
                                       "  vec3 GBufferPosition = ViewSpacePositionFromDepth( uv, texture(uTexture2,uv).x );\n"
                                       "  vec3 GBufferNormal = normalize( texture(uTexture1,uv).xyz );\n"
                                       "  float attenuation =  ( uRadius - length(uLightPositionViewSpace-GBufferPosition) ) / uRadius;\n"
                                       "  float NdotL = max( 0.0, dot( GBufferNormal, normalize(uLightPositionViewSpace - GBufferPosition) ) );\n "
                                       "  color = NdotL * attenuation * vec4(uLightColor,1.0) * texture(uTexture0, uv);\n"
                                       "}\n"
};

vec3 CubicInterpolation( const vec3& p0, const vec3& p1, const vec3&  p2, const vec3&  p3, float progress )
{
  vec3 a3 = 0.5f*p3 - 1.5f*p2 + 1.5f*p1 - 0.5f*p0;
  vec3 a2 = p0 - 2.5f*p1 + 2.0f*p2 - 0.5f*p3;
  vec3 a1 = 0.5f*(p2-p0);

  return progress*progress*progress*a3 + progress*progress*a2 + progress*a1 + p1;
}
}

class App : public Dodo::Application
{
public:
  App()
 :Dodo::Application("Demo",500,500,4,4),
  mTxManager(0),
  mCameraPosition(0.0f,0.0f,50.0f),
  mCameraDirection(0.0f,0.0f,-1.0f),
  mCameraAngle(0.0f,0.0f,0.0f),
  mCameraVelocity(1.0f),
  mMousePosition(0.0f,0.0f),
  mMouseButtonPressed(false),
  mMatrixBuffer(0)
  {}

  ~App()
  {}

  void Init()
  {
    //Load shaders
    mGBufferShader = mRenderManager.AddProgram( (const u8**)gVertexShaderGBuffer, (const u8**)gFragmentShaderGBuffer );
    mColorShader = mRenderManager.AddProgram( (const u8**)gVertexShaderColor, (const u8**)gFragmentShaderColor );
    mPointLightShader = mRenderManager.AddProgram( (const u8**)gVertexShaderPointLight, (const u8**)gFragmentShaderPointLight );

    //Load meshes
    mMesh = mRenderManager.AddMesh( "data/jeep.ms3d" );
    mSphere = mRenderManager.AddMesh( "data/sphere.obj" );

    //Load textures
    mTexture = mRenderManager.Add2DTexture( Image("data/jeep.jpg") );

    mTxManager = new TxManager( OBJECT_COUNT );
    for( u32 i(0); i<OBJECT_COUNT; ++i )
    {
      mObjectTx[i] = mTxManager->CreateTransform( TxComponent(vec3( i*15.0f - 30.0f,0.0f,0.0f),VEC3_ONE,QUAT_UNIT) );
    }

    mTxManager->Update();
    ComputeViewMatrix();

    vec3 lightColors[] = { vec3(1.0f,0.0f,0.0f),vec3(0.0f,1.0f,0.0f),vec3(0.0f,0.0f,1.0f),vec3(1.0f,1.0f,0.0f),vec3(0.0f,1.0f,1.0f) };
    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mLightColor[i] = lightColors[ i % 5 ];
    }

    //Create GBuffer
    mFbo =  mRenderManager.AddFrameBuffer();
    mColorAttachment = mRenderManager.Add2DTexture( Dodo::Image( mWindowSize.x, mWindowSize.y, 0, Dodo::FORMAT_RGBA8 ));
    mNormalAttachment = mRenderManager.Add2DTexture( Dodo::Image( mWindowSize.x, mWindowSize.y, 0, Dodo::FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Dodo::Image( mWindowSize.x, mWindowSize.y, 0, Dodo::FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 1, mNormalAttachment );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );
    mRenderManager.BindFrameBuffer( mFbo );
    u32 buffers[2] = {0u,1u};
    mRenderManager.SetDrawBuffers( 2, &buffers[0]);

    //Set GL state
    mRenderManager.SetCullFace( Dodo::CULL_BACK );
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( Dodo::vec4(0.0f,0.0f,0.0f,0.0f));
    mRenderManager.SetClearDepth( 1.0f );
  }

  void AnimateLights( f32 timeDelta )
  {
    static const vec3 light_path[] =
    {
      vec3(-30.0f,  10.0f, 0.0f),
      vec3( 0.0f,   5.0f, 0.0f),
      vec3( 35.0f,  10.0f, 0.0f),
      vec3( 0.0f,   20.0f, 0.0f),
      vec3(-30.0f,  10.0f, 0.0f)
    };

    static f32 totalTime = 0.0f;
    totalTime += timeDelta*0.001f;

    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      float t = totalTime + i* 5.0f/LIGHT_COUNT;
      int baseFrame = (int)t;
      float f = t - baseFrame;

      vec3 p0 = light_path[(baseFrame + 0) % 5];
      vec3 p1 = light_path[(baseFrame + 1) % 5];
      vec3 p2 = light_path[(baseFrame + 2) % 5];
      vec3 p3 = light_path[(baseFrame + 3) % 5];

      mLightPosition[i] = CubicInterpolation(p0, p1, p2, p3, f);
    }
  }

  void Render()
  {
    //Animate lights
    AnimateLights( GetTimeDelta() );

    //Build GBuffer
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_LESS_EQUAL );
    mRenderManager.BindFrameBuffer( mFbo );
    mRenderManager.ClearBuffers( Dodo::DEPTH_BUFFER );
    mRenderManager.ClearColorAttachment( 0, vec4(0.0f,0.0f,0.0f,0.0f) );
    mRenderManager.ClearColorAttachment( 1, vec4(0.0f,0.0f,0.0f,0.0f) );
    mRenderManager.UseProgram( mGBufferShader );
    mRenderManager.Bind2DTexture( mTexture, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mGBufferShader,"uTexture0"), 0 );
    mRenderManager.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderManager.SetupMeshVertexFormat( mMesh );
    for( u32 i(0); i<OBJECT_COUNT; ++i )
    {
      mat4 modelMatrix;
      mTxManager->GetWorldTransform(mObjectTx[i], &modelMatrix );
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mGBufferShader,"uModelMatrix"), modelMatrix );
      mRenderManager.DrawMesh( mMesh );
    }

    //Draw light volumes to illuminate
    mRenderManager.SetBlendingMode(BLEND_ADD);
    mRenderManager.SetBlendingFunction( BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE );
    mRenderManager.SetDepthTest( Dodo::DEPTH_TEST_ALWAYS );
    mRenderManager.BindFrameBuffer( 0 );
    mRenderManager.ClearBuffers(Dodo::COLOR_BUFFER | Dodo::DEPTH_BUFFER );
    mRenderManager.UseProgram( mPointLightShader );
    mRenderManager.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderManager.Bind2DTexture( mColorAttachment, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uTexture0"), 0 );
    mRenderManager.Bind2DTexture( mNormalAttachment, 1 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uTexture1"), 1 );
    mRenderManager.Bind2DTexture( mDepthStencilAttachment, 2 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uTexture2"), 2 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uRadius"), 25.0f );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uProjectionInverse"), mProjectionI );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y ) );

    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uLightPosition"), mLightPosition[i] );
      vec4 lightPositionViewSpace = vec4(mLightPosition[i].x, mLightPosition[i].y, mLightPosition[i].z, 1.0f ) * mView;
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uLightPositionViewSpace"), lightPositionViewSpace.xyz() );
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mPointLightShader,"uLightColor"), mLightColor[i] );
      mRenderManager.SetupMeshVertexFormat( mSphere );
      mRenderManager.DrawMesh( mSphere );
    }

    //Draw lights
    mRenderManager.SetBlendingMode(BLEND_DISABLED);
    mRenderManager.UseProgram( mColorShader );
    mRenderManager.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderManager.Bind2DTexture( mDepthStencilAttachment, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mColorShader,"uTexture0"), 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mColorShader,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y ) );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mColorShader,"uScale"), 0.5f );
    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mColorShader,"uPosition"), mLightPosition[i]);
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation( mColorShader, "uColor"), mLightColor[i] );
      mRenderManager.SetupMeshVertexFormat( mSphere );
      mRenderManager.DrawMesh( mSphere );
    }
  }

  void ComputeViewMatrix()
  {
    Dodo::quat cameraOrientation =  Dodo::QuaternionFromAxisAngle( Dodo::vec3(0.0f,1.0f,0.0f), mCameraAngle.x ) *
                                    Dodo::QuaternionFromAxisAngle( Dodo::vec3(1.0f,0.0f,0.0f), mCameraAngle.y );

    mCameraDirection = Dodo::Rotate( Dodo::vec3(0.0f,0.0f,1.0f), cameraOrientation );
    mCameraDirection.Normalize();
    Dodo::ComputeInverse( Dodo::ComputeTransform(mCameraPosition, Dodo::VEC3_ONE,  cameraOrientation ), mView );
    Dodo::ComputeInverse( mProjection, mProjectionI );

    UpdateMatrixBuffer();
  }

  void UpdateMatrixBuffer()
  {
    if( mMatrixBuffer == 0 )
    {
      mat4 data[3];
      data[0] = mView;
      data[1] = mProjection;
      data[2] = mView * mProjection;
      mMatrixBuffer = mRenderManager.AddBuffer( sizeof(data), data );
    }
    else
    {
      mat4* bufferData = (mat4*)mRenderManager.MapBuffer( mMatrixBuffer, Dodo::BUFFER_MAP_WRITE );
      if(bufferData)
      {
        bufferData[0] = mView;
        bufferData[1] = mProjection;
        bufferData[2] = mView * mProjection;
        mRenderManager.UnmapBuffer();
      }
    }
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );

    mProjection = Dodo::ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
    Dodo::ComputeInverse( mProjection, mProjectionI );
    UpdateMatrixBuffer();

    //Recreate framebuffer attachments to match new size
    mRenderManager.RemoveTexture(mColorAttachment);
    mRenderManager.RemoveTexture(mNormalAttachment);
    mRenderManager.RemoveTexture(mDepthStencilAttachment);
    mColorAttachment = mRenderManager.Add2DTexture( Dodo::Image( width, height, 0, Dodo::FORMAT_RGBA8 ));
    mNormalAttachment = mRenderManager.Add2DTexture( Dodo::Image( width, height, 0, Dodo::FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderManager.Add2DTexture( Dodo::Image( width, height, 0, Dodo::FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderManager.Attach2DColorTextureToFrameBuffer( mFbo, 1, mNormalAttachment );
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

  TxManager* mTxManager;
  mat4 mProjection;
  mat4 mProjectionI;
  mat4 mView;
  vec3 mCameraPosition;
  vec3 mCameraDirection;
  vec3 mCameraAngle;
  f32 mCameraVelocity;
  vec2 mMousePosition;
  bool mMouseButtonPressed;
  vec3 mLightPosition[LIGHT_COUNT];
  vec3 mLightColor[LIGHT_COUNT];
  TxId mObjectTx[OBJECT_COUNT];
  ProgramId mColorShader;
  ProgramId mGBufferShader;
  ProgramId mPointLightShader;
  ProgramId mLightPass;
  MeshId mMesh;
  MeshId mSphere;
  TextureId mTexture;
  BufferId mMatrixBuffer;
  Dodo::FBOId mFbo;
  Dodo::TextureId mColorAttachment;
  Dodo::TextureId mNormalAttachment;
  Dodo::TextureId mDepthStencilAttachment;

};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

