
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <scene-graph.h>
#include <types.h>
#include <render-manager.h>
#include <material.h>

using namespace Dodo;

namespace
{

const char* gVertexShaderSourceDiffuse[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"

                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "out vec3 lightPosition_cameraSpace;\n"
                                            "out vec3 position_cameraSpace;\n"
                                            "out vec3 normal_cameraSpace;"
                                            "out vec3 viewVector_cameraSpace;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  normal_cameraSpace = normalize( (uModelView*vec4(aNormal,0.0)).xyz);\n"
                                            "  lightPosition_cameraSpace = (uModelView*vec4(0.0,1.0,0.0,1.0)).xyz;\n"
                                            "  position_cameraSpace = (uModelView*vec4(aPosition,1.0)).xyz;\n"
                                            "  viewVector_cameraSpace = -normalize( (uModelView * vec4(aPosition,1.0)).xyz);\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                              "#version 440 core\n"
                                              "in vec3 normal_cameraSpace;\n"
                                              "in vec3 lightPosition_cameraSpace;\n"
                                              "in vec3 position_cameraSpace;\n"
                                              "in vec3 viewVector_cameraSpace;\n"
                                              "uniform vec3 uDiffuseColor;\n"
                                              "uniform vec3 uSpecularColor;\n"
                                              "out vec4 color;\n"
                                              "void main(void)\n"
                                              "{\n"
                                              "  vec3 lightDirection = normalize(lightPosition_cameraSpace - position_cameraSpace);\n "
                                              "  float NdotL = max(0.0, dot(normal_cameraSpace,lightDirection));\n"
                                              "  vec3 reflection = normalize( -reflect( viewVector_cameraSpace, normal_cameraSpace));\n"
                                              "  vec3 specular = pow(max(dot(reflection,lightDirection),0.0),100.0) * uSpecularColor;\n"
                                              "  color = vec4( uDiffuseColor *(NdotL+0.2)  + specular, 1.0);\n"
                                              "}\n"
};


const char* gVertexShaderFullscreen[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"

                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"

                                            "out vec3 viewVector_viewSpace;\n"
                                            "out vec3 normal_viewSpace;"

                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  normal_viewSpace = normalize( (uModelView*vec4(aNormal,0.0)).xyz);\n"
                                            "  viewVector_viewSpace = normalize(-(uModelView*vec4(aPosition,1.0)).xyz);\n"
                                            "}\n"
};
const char* gFragmentShaderFullscreen[] = {
                                              "#version 440 core\n"
                                              "in vec3 normal_viewSpace;\n"
                                              "in vec3 viewVector_viewSpace;\n"
                                              "uniform vec3 uSpecularColor;\n"
                                              "uniform sampler2D uColorBuffer;\n"
                                              "uniform sampler2D uDepthStencilBuffer;\n"
                                              "uniform vec2 uTextureSize;\n"
                                              "uniform mat4 uProjection;\n"
                                              "uniform mat4 uProjectionInverse;\n"
                                              "uniform vec2 uCameraNearFar;\n"
                                              "float stepDelta = 0.01;\n"
                                              "int maxStepCount = 200;\n"
                                              "out vec4 color;\n"

                                             "float LinearizeDepth(float depth, float n, float f)\n"
                                             "{ \n"
                                             "  return (2 * uCameraNearFar.x) / (uCameraNearFar.y + uCameraNearFar.x - depth * (uCameraNearFar.y - uCameraNearFar.x)); \n"
                                             "}\n"

                                              "void main(void)\n"
                                              "{\n"
                                              "  vec2 uv = gl_FragCoord.xy/uTextureSize;\n"
                                              "  if( 2.0*gl_FragCoord.z-1.0 > texture2D( uDepthStencilBuffer, uv).r )"
                                              "  {\n"
                                              "     discard;\n"
                                              "  }\n"
                                              "  if(uSpecularColor.x == 0.0 && uSpecularColor.y == 0.0 && uSpecularColor.z == 0.0 )\n"
                                              "  {\n"
                                              "    color = texture2D( uColorBuffer, uv );\n"
                                              "  }\n"
                                              "  else\n"
                                              "  {\n"
                                              "    vec3 reflectionColor = vec3(0.0,0.0,0.0);\n"
                                              "    vec3 reflectionVectorViewSpace = -reflect( viewVector_viewSpace, normal_viewSpace);\n"
                                              "    vec3 reflectionVectorProjectionSpace = (uProjection * vec4(reflectionVectorViewSpace,0.0)).xyz;\n"
                                              "    reflectionVectorProjectionSpace = reflectionVectorProjectionSpace * stepDelta;\n"
                                              "    vec3 positionInRay = vec3(uv + reflectionVectorProjectionSpace.xy*10.0, 0.0);\n"
                                              "    positionInRay.z = LinearizeDepth(texture2D( uDepthStencilBuffer, positionInRay.xy).r, 0.1, 100);\n"
                                              "    int sampleCount = 0;\n"
                                              "    while(positionInRay.x <= 1.0 && positionInRay.x >= 0.0 && positionInRay.y <= 1.0 && positionInRay.y >= 0.0 && positionInRay.z <= 1.0 && positionInRay.z >= 0.0 && sampleCount < maxStepCount )\n"
                                              "    {\n"
                                              "      positionInRay = positionInRay + reflectionVectorProjectionSpace;\n"
                                              "      float sampledDepth = LinearizeDepth(texture2D(uDepthStencilBuffer, positionInRay.xy).z, 0.1, 100);\n"
                                              "      if(positionInRay.z > sampledDepth)\n"
                                              "      {\n"
                                              "        reflectionColor = texture(uColorBuffer, positionInRay.xy).rgb;\n"
                                              "        break;\n"
                                              "      }"
                                              "      sampleCount++;\n"
                                              "    }\n"
                                              "    color = texture2D( uColorBuffer, uv ) + vec4(reflectionColor* uSpecularColor,1.0);\n"
                                              "  }\n"
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
   mMeshes(0),
   mMaterials(0),
   mMeshCount(0),
   mShader(),
   mCamera( vec3(0.0f,1.0f,3.0f), vec2(0.0f,0.0f), 5.0f ),
   mMousePosition(0.0f,0.0f),
   mMouseButtonPressed(false)
  {}

  ~App()
  {
    delete[] mMeshes;
    delete[] mMaterials;
  }

  void Init()
  {
    //Create a shader
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    mShaderFullScreen = mRenderManager.AddProgram((const u8**)gVertexShaderFullscreen, (const u8**)gFragmentShaderFullscreen);

    //Load meshes
    mMeshCount = mRenderManager.GetMeshCountFromFile("data/cornell-box.obj");
    mMeshes = new MeshId[mMeshCount];
    mMaterials = new Material[mMeshCount];
    mRenderManager.AddMultipleMeshesFromFile("data/cornell-box.obj", mMeshes, mMaterials, mMeshCount );

    //Create offscreen framebuffer
    mOffscreenFrameBuffer =  mRenderManager.AddFrameBuffer();
    mColorBuffer = mRenderManager.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_RGBA8 ), false);
    mDepthStencilBuffer = mRenderManager.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_GL_DEPTH_STENCIL ), false);
    mRenderManager.Attach2DColorTextureToFrameBuffer( mOffscreenFrameBuffer, 0, mColorBuffer );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mOffscreenFrameBuffer, mDepthStencilBuffer );
    mFullscreenQuad = mRenderManager.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Set GL state
    mRenderManager.SetCullFace( CULL_BACK );
    mRenderManager.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderManager.SetClearColor( vec4(0.0f,0.0f,0.0f,1.0f));
    mRenderManager.SetClearDepth( 1.0f );
  }

  void Render()
  {
    //Draw scene to an offscreen buffer
    mRenderManager.BindFrameBuffer( mOffscreenFrameBuffer );
    mRenderManager.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );
    mRenderManager.UseProgram( mShader );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"), mCamera.txInverse );

    for( u32 i(0); i<mMeshCount; ++i )
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uDiffuseColor"), mMaterials[i].mDiffuseColor );
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uSpecularColor"), mMaterials[i].mSpecularColor );
      mRenderManager.SetupMeshVertexFormat( mMeshes[i] );
      mRenderManager.DrawMesh( mMeshes[i] );
    }

    //Draw specular reflections using Screen Space raytracing
    mRenderManager.BindFrameBuffer( 0 );
    mRenderManager.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );
    mRenderManager.UseProgram( mShaderFullScreen );

    //Setup uniforms
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uModelViewProjection"), mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uModelView"), mCamera.txInverse );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uProjection"), mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uProjectionInverse"), mProjectionInverse );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uCameraNearFar"), vec2(0.1f,100.0f) );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y )  );

    //Bind textures
    mRenderManager.Bind2DTexture( mColorBuffer, 0 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uColorBuffer"), 0 );
    mRenderManager.Bind2DTexture( mDepthStencilBuffer, 1 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uDepthStencilBuffer"), 1 );
    for( u32 i(0); i<mMeshCount; ++i )
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShaderFullScreen,"uSpecularColor"), mMaterials[i].mSpecularColor );
      mRenderManager.SetupMeshVertexFormat( mMeshes[i] );
      mRenderManager.DrawMesh( mMeshes[i] );
    }
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderManager.SetViewport( 0, 0, width, height );
    mProjection = ComputeProjectionMatrix( 1.5f,(f32)width / (f32)height,0.1f,100.0f );
    ComputeInverse(mProjection,mProjectionInverse);

    //Re-do frame buffer attachments to match new window size
    mRenderManager.RemoveTexture(mColorBuffer);
    mRenderManager.RemoveTexture(mDepthStencilBuffer);
    mColorBuffer = mRenderManager.Add2DTexture( Image( width, height, 0, FORMAT_RGBA8 ));
    mDepthStencilBuffer = mRenderManager.Add2DTexture( Image( width, height, 0, FORMAT_GL_DEPTH_STENCIL ));
    mRenderManager.Attach2DColorTextureToFrameBuffer( mOffscreenFrameBuffer, 0, mColorBuffer );
    mRenderManager.AttachDepthStencilTextureToFrameBuffer( mOffscreenFrameBuffer, mDepthStencilBuffer );
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

  MeshId* mMeshes;
  Material* mMaterials;
  u32 mMeshCount;

  FBOId mOffscreenFrameBuffer;
  TextureId mColorBuffer;
  TextureId mDepthStencilBuffer;
  MeshId mFullscreenQuad;

  ProgramId mShader;
  ProgramId mShaderFullScreen;

  FreeCamera mCamera;
  mat4 mProjection;
  mat4 mProjectionInverse;
  vec2 mMousePosition;
  bool mMouseButtonPressed;
};

int main()
{
  App app;
  app.MainLoop();

  return 0;
}

