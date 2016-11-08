
#include <gl-application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <types.h>
#include <gl-renderer.h>
#include <material.h>
#include <camera.h>

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
                                              "float stepDelta = 0.02;\n"
                                              "int maxStepCount = 200;\n"
                                              "out vec4 color;\n"

                                             "float LinearizeDepth(float depth)\n"
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
                                              "    positionInRay.z = LinearizeDepth(texture2D( uDepthStencilBuffer, positionInRay.xy).r);\n"
                                              "    int sampleCount = 0;\n"
                                              "    while(positionInRay.x <= 1.0 && positionInRay.x >= 0.0 && positionInRay.y <= 1.0 && positionInRay.y >= 0.0 && positionInRay.z <= 1.0 && positionInRay.z >= 0.0 && sampleCount < maxStepCount )\n"
                                              "    {\n"
                                              "      positionInRay = positionInRay + reflectionVectorProjectionSpace;\n"
                                              "      float sampledDepth = LinearizeDepth(texture2D(uDepthStencilBuffer, positionInRay.xy).z);\n"
                                              "      if(abs(positionInRay.z - sampledDepth) < 0.03)\n"
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

}

class App : public GLApplication
{
public:

  App()
  :GLApplication("Demo",500,500,4,4),
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
    mShader = mRenderer.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    mShaderFullScreen = mRenderer.AddProgram((const u8**)gVertexShaderFullscreen, (const u8**)gFragmentShaderFullscreen);

    //Load meshes
    mMeshCount = mRenderer.GetMeshCountFromFile("../resources/cornell-box.obj");
    mMeshes = new MeshId[mMeshCount];
    mMaterials = new Material[mMeshCount];
    mRenderer.AddMultipleMeshesFromFile("../resources/cornell-box.obj", mMeshes, mMaterials, mMeshCount );

    //Create offscreen framebuffer
    mOffscreenFrameBuffer =  mRenderer.AddFrameBuffer();
    mColorBuffer = mRenderer.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_RGBA8 ), false);
    mDepthStencilBuffer = mRenderer.Add2DTexture( Image( mWindowSize.x, mWindowSize.y, 0, FORMAT_GL_DEPTH_STENCIL ), false);
    mRenderer.Attach2DColorTextureToFrameBuffer( mOffscreenFrameBuffer, 0, mColorBuffer );
    mRenderer.AttachDepthStencilTextureToFrameBuffer( mOffscreenFrameBuffer, mDepthStencilBuffer );
    mFullscreenQuad = mRenderer.CreateQuad(Dodo::uvec2(2u,2u),true,false );

    //Set GL state
    mRenderer.SetCullFace( CULL_BACK );
    mRenderer.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderer.SetClearColor( vec4(0.0f,0.0f,0.0f,1.0f));
    mRenderer.SetClearDepth( 1.0f );
  }

  void Render()
  {
    //Draw scene to an offscreen buffer
    mRenderer.BindFrameBuffer( mOffscreenFrameBuffer );
    mRenderer.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );
    mRenderer.UseProgram( mShader );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelViewProjection"), mCamera.txInverse * mProjection );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelView"), mCamera.txInverse );

    for( u32 i(0); i<mMeshCount; ++i )
    {
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uDiffuseColor"), mMaterials[i].mDiffuseColor );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uSpecularColor"), mMaterials[i].mSpecularColor );
      mRenderer.SetupMeshVertexFormat( mMeshes[i] );
      mRenderer.DrawMesh( mMeshes[i] );
    }

    //Draw specular reflections using Screen Space raytracing
    mRenderer.BindFrameBuffer( 0 );
    mRenderer.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );
    mRenderer.UseProgram( mShaderFullScreen );

    //Setup uniforms
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uModelViewProjection"), mCamera.txInverse * mProjection );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uModelView"), mCamera.txInverse );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uProjection"), mProjection );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uProjectionInverse"), mProjectionInverse );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uCameraNearFar"), vec2(0.1f,100.0f) );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uTextureSize"), vec2( mWindowSize.x, mWindowSize.y )  );

    //Bind textures
    mRenderer.Bind2DTexture( mColorBuffer, 0 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uColorBuffer"), 0 );
    mRenderer.Bind2DTexture( mDepthStencilBuffer, 1 );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uDepthStencilBuffer"), 1 );
    for( u32 i(0); i<mMeshCount; ++i )
    {
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uSpecularColor"), mMaterials[i].mSpecularColor );
      mRenderer.SetupMeshVertexFormat( mMeshes[i] );
      mRenderer.DrawMesh( mMeshes[i] );
    }
  }

  void OnResize(size_t width, size_t height )
  {
    mRenderer.SetViewport( 0, 0, width, height );
    mProjection = ComputePerspectiveProjectionMatrix( 1.5f,(f32)width / (f32)height,0.1f,100.0f );
    ComputeInverse(mProjection,mProjectionInverse);

    //Re-do frame buffer attachments to match new window size
    mRenderer.RemoveTexture(mColorBuffer);
    mRenderer.RemoveTexture(mDepthStencilBuffer);
    mColorBuffer = mRenderer.Add2DTexture( Image( width, height, 0, FORMAT_RGBA8 ));
    mDepthStencilBuffer = mRenderer.Add2DTexture( Image( width, height, 0, FORMAT_GL_DEPTH_STENCIL ));
    mRenderer.Attach2DColorTextureToFrameBuffer( mOffscreenFrameBuffer, 0, mColorBuffer );
    mRenderer.AttachDepthStencilTextureToFrameBuffer( mOffscreenFrameBuffer, mDepthStencilBuffer );
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
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, 0.01f );
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.Move( 0.01f, 0.0f );
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -0.01f, 0.0f );
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

