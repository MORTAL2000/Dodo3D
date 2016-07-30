
#include <gl-application.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>
#include <camera.h>

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
                                           "  vec3 reflection = normalize( -reflect( vec3(0.0,0.0,1.0), GBufferNormal));\n"
                                           "  float specular = pow(max(dot(reflection,normalize(uLightPositionViewSpace - GBufferPosition)),0.0),100.0);\n"
                                           "  color = attenuation * ( NdotL * vec4(uLightColor,1.0) * texture(uTexture0, uv) + specular*vec4(uLightColor,1.0) );\n"
                                           "}\n"
};
}

class App : public GLApplication
{
public:
  App()
:GLApplication("Demo",500,500,4,4),
 mTxManager( OBJECT_COUNT ),
 mCamera( vec3(0.0f,0.0f,50.0f), vec2(0.0f,0.0f), 100.0f ),
 mMousePosition(0.0f,0.0f),
 mMouseButtonPressed(false),
 mMatrixBuffer(0)
{}

  ~App()
  {}

  void Init()
  {
    //Load shaders
    mGBufferShader = mRenderer.AddProgram( (const u8**)gVertexShaderGBuffer, (const u8**)gFragmentShaderGBuffer );
    mColorShader = mRenderer.AddProgram( (const u8**)gVertexShaderColor, (const u8**)gFragmentShaderColor );
    mPointLightShader = mRenderer.AddProgram( (const u8**)gVertexShaderPointLight, (const u8**)gFragmentShaderPointLight );

    //Load meshes
    mMesh = mRenderer.AddMeshFromFile( "../resources/jeep.ms3d" );
    mSphere = mRenderer.AddMeshFromFile( "../resources/sphere.obj" );

    //Load textures
    mTexture = mRenderer.Add2DTexture( Image("../resources/jeep.jpg") );

    for( u32 i(0); i<OBJECT_COUNT; ++i )
    {
      mObjectTx[i] = mTxManager.CreateTransform( TxComponent(vec3( i*15.0f - 30.0f,0.0f,0.0f),VEC3_ONE,QUAT_UNIT) );
    }

    mTxManager.Update();

    vec3 lightColors[] = { vec3(1.0f,0.0f,0.0f),vec3(0.0f,1.0f,0.0f),vec3(0.0f,0.0f,1.0f),vec3(1.0f,1.0f,0.0f),vec3(0.0f,1.0f,1.0f) };
    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mLightColor[i] = lightColors[ i % 5 ];
    }

    //Create GBuffer
    mFbo =  mRenderer.AddFrameBuffer();
    mColorAttachment = mRenderer.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_RGBA8 ), false);
    mNormalAttachment = mRenderer.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_RGBA8 ), false);
    mDepthStencilAttachment = mRenderer.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_GL_DEPTH_STENCIL ), false);
    mRenderer.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderer.Attach2DColorTextureToFrameBuffer( mFbo, 1, mNormalAttachment );
    mRenderer.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );
    mRenderer.BindFrameBuffer( mFbo );
    u32 buffers[2] = {0u,1u};
    mRenderer.SetDrawBuffers( 2, &buffers[0]);

    //Set GL state
    mRenderer.SetCullFace( CULL_BACK );
    mRenderer.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderer.SetClearColor( vec4(0.0f,0.0f,0.0f,0.0f));
    mRenderer.SetClearDepth( 1.0f );
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
    mRenderer.SetCullFace( CULL_BACK );
    //Build GBuffer
    mRenderer.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderer.BindFrameBuffer( mFbo );
    mRenderer.ClearBuffers( DEPTH_BUFFER );
    mRenderer.ClearColorAttachment( 0, vec4(0.0f,0.0f,0.0f,0.0f) );
    mRenderer.ClearColorAttachment( 1, vec4(0.0f,0.0f,0.0f,0.0f) );
    mRenderer.UseProgram( mGBufferShader );
    mRenderer.Bind2DTexture( mTexture, 0 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mGBufferShader,"uTexture0"), 0 );
    mRenderer.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderer.SetupMeshVertexFormat( mMesh );
    for( u32 i(0); i<OBJECT_COUNT; ++i )
    {
      mat4 modelMatrix;
      mTxManager.GetWorldTransform(mObjectTx[i], &modelMatrix );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mGBufferShader,"uModelMatrix"), modelMatrix );
      mRenderer.DrawMesh( mMesh );
    }

    //Draw light volumes to illuminate
    mRenderer.SetCullFace( CULL_FRONT );
    mRenderer.SetBlendingMode(BLEND_ADD);
    mRenderer.SetBlendingFunction( BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE, BLENDING_FUNCTION_ONE );
    mRenderer.SetDepthTest( DEPTH_TEST_ALWAYS );
    mRenderer.BindFrameBuffer( 0 );
    mRenderer.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );
    mRenderer.UseProgram( mPointLightShader );
    mRenderer.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderer.Bind2DTexture( mColorAttachment, 0 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uTexture0"), 0 );
    mRenderer.Bind2DTexture( mNormalAttachment, 1 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uTexture1"), 1 );
    mRenderer.Bind2DTexture( mDepthStencilAttachment, 2 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uTexture2"), 2 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uRadius"), 25.0f );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uProjectionInverse"), mProjectionInverse );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y ) );
    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uLightPosition"), mLightPosition[i] );
      vec4 lightPositionViewSpace = vec4(mLightPosition[i].x, mLightPosition[i].y, mLightPosition[i].z, 1.0f ) * mCamera.txInverse;
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uLightPositionViewSpace"), lightPositionViewSpace.xyz() );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mPointLightShader,"uLightColor"), mLightColor[i] );
      mRenderer.SetupMeshVertexFormat( mSphere );
      mRenderer.DrawMesh( mSphere );
    }

    //Draw lights
    mRenderer.SetCullFace( CULL_BACK );
    mRenderer.SetBlendingMode(BLEND_DISABLED);
    mRenderer.UseProgram( mColorShader );
    mRenderer.BindUniformBuffer( mMatrixBuffer, 0 );
    mRenderer.Bind2DTexture( mDepthStencilAttachment, 0 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mColorShader,"uTexture0"), 0 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mColorShader,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y ) );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mColorShader,"uScale"), 0.5f );
    for( u32 i(0); i<LIGHT_COUNT; ++i )
    {
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mColorShader,"uPosition"), mLightPosition[i]);
      mRenderer.SetUniform( mRenderer.GetUniformLocation( mColorShader, "uColor"), mLightColor[i] );
      mRenderer.SetupMeshVertexFormat( mSphere );
      mRenderer.DrawMesh( mSphere );
    }
  }

  void UpdateMatrixBuffer()
  {

    if( mMatrixBuffer == 0 )
    {
      mat4 data[3];
      data[0] = mCamera.txInverse;
      data[1] = mProjection;
      data[2] = mCamera.txInverse * mProjection;
      mMatrixBuffer = mRenderer.AddBuffer( sizeof(data), data );
    }
    else
    {
      mat4* bufferData = (mat4*)mRenderer.MapBuffer( mMatrixBuffer, BUFFER_MAP_WRITE );
      if(bufferData)
      {
        bufferData[0] = mCamera.txInverse;
        bufferData[1] = mProjection;
        bufferData[2] = mCamera.txInverse * mProjection;
        mRenderer.UnmapBuffer();
      }
    }
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderer.SetViewport( 0, 0, width, height );

    mProjection = ComputePerspectiveProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
    ComputeInverse( mProjection, mProjectionInverse );
    UpdateMatrixBuffer();

    //Recreate framebuffer attachments to match new size
    mRenderer.RemoveTexture(mColorAttachment);
    mRenderer.RemoveTexture(mNormalAttachment);
    mRenderer.RemoveTexture(mDepthStencilAttachment);
    mColorAttachment = mRenderer.Add2DTexture( Image( width, height, 0, FORMAT_RGBA8 ));
    mNormalAttachment = mRenderer.Add2DTexture( Image( width, height, 0, FORMAT_RGBA8 ));
    mDepthStencilAttachment = mRenderer.Add2DTexture( Image( width, height, 0, FORMAT_GL_DEPTH_STENCIL ));
    mRenderer.Attach2DColorTextureToFrameBuffer( mFbo, 0, mColorAttachment );
    mRenderer.Attach2DColorTextureToFrameBuffer( mFbo, 1, mNormalAttachment );
    mRenderer.AttachDepthStencilTextureToFrameBuffer( mFbo, mDepthStencilAttachment );
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
          mCamera.Move( 0.0f, -0.01f);
          UpdateMatrixBuffer();
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, 0.01f );
          UpdateMatrixBuffer();
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.Move( 0.01f, 0.0f );
          UpdateMatrixBuffer();
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -0.01f, 0.0f );
          UpdateMatrixBuffer();
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

      mCamera.Rotate( angleX, angleY );

      mMousePosition.x = x;
      mMousePosition.y = y;
      UpdateMatrixBuffer();
    }
  }

private:

  TxManager mTxManager;
  FreeCamera mCamera;

  mat4 mProjection;
  mat4 mProjectionInverse;

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
  FBOId mFbo;
  TextureId mColorAttachment;
  TextureId mNormalAttachment;
  TextureId mDepthStencilAttachment;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

