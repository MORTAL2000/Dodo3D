
#include <gl-application.h>
#include <maths.h>
#include <material.h>

using namespace Dodo;

namespace
{

const char* gVSDepthPass[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "}\n"
};

const char* gVertexShaderSourceDiffuse[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"

                                            "uniform mat4 uView;\n"
                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "uniform mat4 uModelLightViewProjection;\n"
                                            "uniform vec3 uLightDirection;\n"
                                            "out vec3 lightDirection_cameraSpace;\n"
                                            "out vec3 position_cameraSpace;\n"
                                            "out vec3 normal_cameraSpace;"
                                            "out vec3 viewVector_cameraSpace;\n"
                                            "out vec4 position_LightClipSpace;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  normal_cameraSpace = normalize( (uModelView*vec4(aNormal,0.0)).xyz);\n"
                                            "  lightDirection_cameraSpace = (uView*vec4(-uLightDirection,0.0)).xyz;\n"
                                            "  position_cameraSpace = (uModelView*vec4(aPosition,1.0)).xyz;\n"
                                            "  viewVector_cameraSpace = -normalize( (uModelView * vec4(aPosition,1.0)).xyz);\n"
                                            "  position_LightClipSpace = uModelLightViewProjection * vec4(aPosition,1.0);\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                              "#version 440 core\n"
                                              "in vec3 normal_cameraSpace;\n"
                                              "in vec3 lightDirection_cameraSpace;\n"
                                              "in vec3 position_cameraSpace;\n"
                                              "in vec3 viewVector_cameraSpace;\n"
                                              "in vec4 position_LightClipSpace;\n"
                                              "uniform vec3 uDiffuseColor;\n"
                                              "uniform vec3 uSpecularColor;\n"
                                              "uniform uvec2 shadowMapSize;\n"
                                              "uniform sampler2D uShadowMap;\n"
                                              "out vec4 color;\n"
                                              "void main(void)\n"
                                              "{\n"
                                              "  float NdotL = max(0.0, dot(normal_cameraSpace,lightDirection_cameraSpace));\n"
                                              "  vec3 reflection = normalize( -reflect( viewVector_cameraSpace, normal_cameraSpace));\n"
                                              "  vec3 specular = pow(max(dot(reflection,lightDirection_cameraSpace),0.0),100.0) * uSpecularColor;\n"
                                              "  color = vec4( uDiffuseColor *(NdotL+0.2)  + specular, 1.0);\n"

                                              "  vec2 depthMapUV = 0.5 * (position_LightClipSpace.xy/position_LightClipSpace.w)+ 0.5;\n"
                                              "  float pixelDepth = 0.5*(position_LightClipSpace.z/position_LightClipSpace.w) + 0.5;"

                                              "  if( pixelDepth >= 0.0 && pixelDepth <= 1.0 && depthMapUV.x >= 0.0 && depthMapUV.x <= 1.0 && depthMapUV.y >= 0.0 && depthMapUV.y <= 1.0 )\n"
                                              "  {\n"
                                              "    ivec2 texel = ivec2(depthMapUV*shadowMapSize);\n"
                                              "    float bias = 0.003*tan(acos(NdotL));"
                                              "    if( texelFetch( uShadowMap, texel, 0).r + bias < pixelDepth ){ color *= 0.5; }\n"
                                              "  }\n"
                                              "}\n"
};

const char* gVertexShaderSourceDiffusePCF[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"

                                            "uniform mat4 uView;\n"
                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "uniform mat4 uModelLightViewProjection;\n"
                                            "uniform vec3 uLightDirection;\n"
                                            "out vec3 lightDirection_cameraSpace;\n"
                                            "out vec3 position_cameraSpace;\n"
                                            "out vec3 normal_cameraSpace;"
                                            "out vec3 viewVector_cameraSpace;\n"
                                            "out vec4 position_LightClipSpace;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  normal_cameraSpace = normalize( (uModelView*vec4(aNormal,0.0)).xyz);\n"
                                            "  lightDirection_cameraSpace = (uView*vec4(-uLightDirection,0.0)).xyz;\n"
                                            "  position_cameraSpace = (uModelView*vec4(aPosition,1.0)).xyz;\n"
                                            "  viewVector_cameraSpace = -normalize( (uModelView * vec4(aPosition,1.0)).xyz);\n"
                                            "  position_LightClipSpace = uModelLightViewProjection * vec4(aPosition,1.0);\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffusePCF[] = {
                                              "#version 440 core\n"
                                              "in vec3 normal_cameraSpace;\n"
                                              "in vec3 lightDirection_cameraSpace;\n"
                                              "in vec3 position_cameraSpace;\n"
                                              "in vec3 viewVector_cameraSpace;\n"
                                              "in vec4 position_LightClipSpace;\n"
                                              "uniform vec3 uDiffuseColor;\n"
                                              "uniform vec3 uSpecularColor;\n"
                                              "uniform uvec2 shadowMapSize;\n"
                                              "uniform sampler2D uShadowMap;\n"
                                              "out vec4 color;\n"

                                              "float SampleShadowMap( ivec2 pixelCoordinates, float pixelDepth, float bias )\n"
                                              "{\n"
                                              "    if( texelFetch( uShadowMap, pixelCoordinates, 0).r + bias < pixelDepth )"
                                              "    {\n"
                                              "      return 0.5;\n"
                                              "    }\n"
                                              "    return 1.0;\n"
                                              "}\n"

                                              "void main(void)\n"
                                              "{\n"
                                              "  float NdotL = max(0.0, dot(normal_cameraSpace,lightDirection_cameraSpace));\n"
                                              "  vec3 reflection = normalize( -reflect( viewVector_cameraSpace, normal_cameraSpace));\n"
                                              "  vec3 specular = pow(max(dot(reflection,lightDirection_cameraSpace),0.0),100.0) * uSpecularColor;\n"
                                              "  color = vec4( uDiffuseColor *(NdotL+0.2)  + specular, 1.0);\n"

                                              "  float bias = 0.003*tan(acos(NdotL));"
                                              "  vec3 pointInLightClipSpace = 0.5 * (position_LightClipSpace.xyz/position_LightClipSpace.w)+ 0.5;\n"
                                              "  ivec2 pixelCoordinates = ivec2( pointInLightClipSpace.xy * shadowMapSize );\n"
                                              "  float pcf = (SampleShadowMap(pixelCoordinates, pointInLightClipSpace.z, bias ) + "
                                              "              SampleShadowMap(pixelCoordinates+ivec2(1,0), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(-1,0), pointInLightClipSpace.z, bias ) +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(0,1), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(0,-1), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(1,1), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(-1,1), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(-1,-1), pointInLightClipSpace.z, bias )  +"
                                              "              SampleShadowMap(pixelCoordinates+ivec2(1,-1), pointInLightClipSpace.z, bias )) / 9.0;"
                                              "  color *= pcf;\n"
                                              "}\n"
};

const char* gVSFullScreen[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "out vec2 uv;\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);"
                                            "  gl_Position = vec4(uv * vec2(2.0f, 2.0f) + vec2(-1.0f, -1.0f), 0.0f, 1.0f);\n"
                                            "}\n"
};
const char* gFSFullScreen[] = {
                                            "#version 440 core\n"
                                            "in vec2 uv;\n"
                                            "uniform sampler2D uTexture;\n"
                                            "out vec4 color;\n"
                                            "float LinearizeDepth(float depth, float near, float far)\n"
                                            "{ \n"
                                            "  return (2.0 * near) / (far + near - depth * (far - near)); \n"
                                            "}\n"
                                            "void main(void)\n"
                                            "{\n"
                                            "  float depth = LinearizeDepth( texture( uTexture,uv ).r, 1.0, 5.0);\n"
                                            "  color = vec4(depth, depth, depth, 1.0);\n"
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

  void Rotate( f32 angleX, f32 angleY )
  {
    mAngle.x =  mAngle.x - angleY;
    angleY = mAngle.y - angleX;
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

class App : public GLApplication
{
public:

  App()
  :GLApplication("Demo",500,500,4,4),
   mMeshes(0),
   mMaterials(0),
   mMeshCount(0),
   mShadowMapSize(1024u,1024u),
   mRenderShadowMap(false),
   mEnablePCF(true),
   mShader(),
   mCamera( vec3(0.0f,1.0f,1.5f), vec2(-0.1f,0.0f), 5.0f ),
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

    vec3 v0(0.0f, 4.0f, 0.0f );
    vec3 v1(3.0f, 0.0f, 0.0f );
    vec3 v = Cross(v0, v1);
    v.Normalize();

    mShader = mRenderer.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    mShaderPCF = mRenderer.AddProgram((const u8**)gVertexShaderSourceDiffusePCF, (const u8**)gFragmentShaderSourceDiffusePCF);
    mShaderDepth = mRenderer.AddProgram((const u8**)gVSDepthPass, 0 );
    mShaderFullScreen = mRenderer.AddProgram((const u8**)gVSFullScreen, (const u8**)gFSFullScreen);

    //Load meshes
    mMeshCount = mRenderer.GetMeshCountFromFile("../resources/cornell-box.obj");
    mMeshes = new MeshId[mMeshCount];
    mMaterials = new Material[mMeshCount];
    mRenderer.AddMultipleMeshesFromFile("../resources/cornell-box.obj", mMeshes, mMaterials, mMeshCount );

    //Create offscreen framebuffer
    mOffscreenFrameBuffer =  mRenderer.AddFrameBuffer();
    mDepthStencilBuffer = mRenderer.Add2DTexture( Image( mShadowMapSize.x, mShadowMapSize.y, 0, FORMAT_GL_DEPTH_STENCIL ), false);
    mRenderer.AttachDepthStencilTextureToFrameBuffer( mOffscreenFrameBuffer, mDepthStencilBuffer );
    mRenderer.BindFrameBuffer( mOffscreenFrameBuffer );
    mRenderer.SetDrawBuffers( 0, 0 );

    //Compute orthographic projection for shadow map generation
    mLightProjectionMatrix = ComputeOrthographicProjectionMatrix( -4.0f, 4.0f, -4.0f, 4.0f, 0.5f, 10.0f );

    //Set GL state
    mRenderer.SetCullFace( CULL_BACK );
    mRenderer.SetDepthTest( DEPTH_TEST_LESS_EQUAL );
    mRenderer.SetClearColor( vec4(0.0f,0.0f,0.0f,1.0f));
    mRenderer.SetClearDepth( 1.0f );
  }

  void AnimateLight( f32 timeDelta )
  {
    static const vec3 light_path[] =
    {
     vec3(3.0f, 4.0f, 0.0f),
     vec3(0.0f, 4.3f, 1.5f),
     vec3(-3.0f,4.0f, 0.0f),
     vec3(0.0f, 4.3f, -1.5f)
    };

    static f32 totalTime = 0.0f;
    totalTime += timeDelta*0.001f;

    int baseFrame = (int)totalTime;
    float f = totalTime - baseFrame;
    vec3 p0 = light_path[(baseFrame + 0) % 4];
    vec3 p1 = light_path[(baseFrame + 1) % 4];
    vec3 p2 = light_path[(baseFrame + 2) % 4];
    vec3 p3 = light_path[(baseFrame + 3) % 4];

    vec3 lightPosition = CubicInterpolation(p0, p1, p2, p3, f);

    //Compute light inverse model matrix
    mLightDirection = vec3(0.0f,0.0f,0.0f) - lightPosition;
    mLightDirection.Normalize();
    quat q( mLightDirection, vec3(0.0f,0.0f,1.0f) );
    mat4 lightMatrix = ComputeTransform( lightPosition, VEC3_ONE, q);
    ComputeInverse( lightMatrix,  mLightViewMatrix );
  }

  void Render()
  {
    AnimateLight(GetTimeDelta());

    //Draw occluders as seen from the light into the depth buffer
    mRenderer.SetCullFace( CULL_FRONT );
    mRenderer.SetViewport( 0, 0, mShadowMapSize.x, mShadowMapSize.y );
    mRenderer.BindFrameBuffer( mOffscreenFrameBuffer );
    mRenderer.ClearBuffers( DEPTH_BUFFER );
    mRenderer.UseProgram( mShader );
    mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelViewProjection"), mLightViewMatrix * mLightProjectionMatrix );
    for( u32 i(0); i<2; ++i )
    {
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uDiffuseColor"), mMaterials[i].mDiffuseColor );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uSpecularColor"), mMaterials[i].mSpecularColor );
      mRenderer.SetupMeshVertexFormat( mMeshes[i] );
      mRenderer.DrawMesh( mMeshes[i] );
    }

    mRenderer.BindFrameBuffer( 0 );
    mRenderer.SetCullFace( CULL_BACK );
    mRenderer.SetViewport( 0, 0, mWindowSize.x, mWindowSize.y );
    mRenderer.ClearBuffers( COLOR_BUFFER | DEPTH_BUFFER );

    if( mRenderShadowMap )
    {
      //Render the shadow map
      mRenderer.UseProgram( mShaderFullScreen );
      mRenderer.Bind2DTexture( mDepthStencilBuffer, 0 );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uTexture0"), mDepthStencilBuffer );
      mRenderer.DrawCall( 3 );
    }
    else
    {
      //Render the scene using the shadow map
      if( mEnablePCF )
      {
        mRenderer.UseProgram( mShaderPCF );
      }
      else
      {
        mRenderer.UseProgram( mShader );
      }

      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uView"), mCamera.txInverse );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelViewProjection"), mCamera.txInverse * mProjection );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelLightViewProjection"), mLightViewMatrix * mLightProjectionMatrix );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uModelView"), mCamera.txInverse );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"shadowMapSize"), mShadowMapSize );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uLightDirection"), mLightDirection );
      mRenderer.Bind2DTexture( mDepthStencilBuffer, 0 );
      mRenderer.SetUniform( mRenderer.GetUniformLocation(mShaderFullScreen,"uShadowMap"), 0 );

      for( u32 i(0); i<mMeshCount; ++i )
      {
        mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uDiffuseColor"), mMaterials[i].mDiffuseColor );
        mRenderer.SetUniform( mRenderer.GetUniformLocation(mShader,"uSpecularColor"), mMaterials[i].mSpecularColor );
        mRenderer.SetupMeshVertexFormat( mMeshes[i] );
        mRenderer.DrawMesh( mMeshes[i] );
      }
    }
  }

  void OnResize(size_t width, size_t height )
  {
    mProjection = ComputePerspectiveProjectionMatrix( 1.5f,(f32)width / (f32)height,0.1f,100.0f );
    ComputeInverse(mProjection,mProjectionInverse);
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
          mCamera.Move( 0.0f, -0.05f);
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( 0.0f, 0.05f );
          break;
        }
        case KEY_LEFT:
        case 'a':
        {
          mCamera.Move( 0.05f, 0.0f );
          break;
        }
        case KEY_RIGHT:
        case 'd':
        {
          mCamera.Move( -0.05f, 0.0f );
          break;
        }
        case KEY_X:
        {
          mEnablePCF = !mEnablePCF;
          break;
        }
        case KEY_Z:
        {
          mRenderShadowMap = !mRenderShadowMap;
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
  TextureId mDepthStencilBuffer;

  mat4 mLightViewMatrix;
  mat4 mLightProjectionMatrix;
  vec3 mLightDirection;
  uvec2 mShadowMapSize;
  bool mRenderShadowMap;
  bool mEnablePCF;

  ProgramId mShader;
  ProgramId mShaderPCF;
  ProgramId mShaderDepth;
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

