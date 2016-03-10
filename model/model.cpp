
#include <application.h>
#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <scene-graph.h>
#include <types.h>
#include <render-manager.h>
#include <camera.h>

using namespace Dodo;

namespace
{

const char* gVertexShaderSourceDiffuse[] = {
                                            "#version 440 core\n"
                                            "layout (location = 0 ) in vec3 aPosition;\n"
                                            "layout (location = 1 ) in vec2 aTexCoord;\n"
                                            "layout (location = 2 ) in vec3 aNormal;\n"
                                            "layout (location = 4 ) in vec3 aTangent;\n"
                                            "layout (location = 5 ) in vec3 aBitangent;\n"

                                            "out vec3 lightDirection_tangentspace;\n"
                                            "out vec3 viewVector_tangentspace;\n"
                                            "out vec2 uv;\n"
                                            "out vec3 shDiffuse;\n"
                                            "uniform mat4 uModelView;\n"
                                            "uniform mat4 uModelViewProjection;\n"
                                            "uniform mat4 uModel;\n"
                                            "uniform vec3 shCoeff[9];\n"

                                            "vec3 ApplySHLights( vec3 normal )\n"
                                            "{\n"
                                            "  vec3 colour = shCoeff[0];\n"
                                            "  colour += shCoeff[1] * normal.x;\n"
                                            "  colour += shCoeff[2] * normal.y;\n"
                                            "  colour += shCoeff[3] * normal.z;\n"
                                            "  colour += shCoeff[4] * (normal.x * normal.z);\n"
                                            "  colour += shCoeff[5] * (normal.z * normal.y);\n"
                                            "  colour += shCoeff[6] * (normal.y * normal.x);\n"
                                            "  colour += shCoeff[7] * (3.0f * normal.z * normal.z - 1.0f);\n"
                                            "  colour += shCoeff[8] * (normal.x * normal.x - normal.y * normal.y);\n"
                                            "  return colour;\n"
                                            "}\n"

                                            "void main(void)\n"
                                            "{\n"
                                            "  gl_Position = uModelViewProjection * vec4(aPosition,1.0);\n"
                                            "  vec3 normal_cameraspace = (uModelView * vec4(aNormal,0.0)).xyz;\n"
                                            "  vec3 lightDirection_cameraspace = normalize((uModelView * vec4( 1.0,0.0,0.0,0.0)).xyz);\n"
                                            "  vec3 viewVector = -normalize( (uModelView * vec4(aPosition,1.0)).xyz);\n"


                                            "  vec3 vertexTangent_cameraspace = (uModelView * vec4(aTangent,0.0)).xyz;\n"
                                            "  vec3 vertexBitangent_cameraspace = (uModelView * vec4(aBitangent,0.0)).xyz;\n"
                                            "  mat3 TBN = transpose(mat3(vertexTangent_cameraspace,vertexBitangent_cameraspace, normal_cameraspace));\n"
                                            "  lightDirection_tangentspace = normalize( TBN * lightDirection_cameraspace );\n"
                                            "  viewVector_tangentspace  = normalize( TBN * viewVector);\n"
                                            "  uv = aTexCoord;\n"
                                            "  vec3 normal_worldspace = normalize( (uModel * vec4(aNormal,0.0)).xyz );\n"
                                            "  shDiffuse = ApplySHLights( normal_worldspace );\n"
                                            "}\n"
};
const char* gFragmentShaderSourceDiffuse[] = {
                                              "#version 440 core\n"
                                              "uniform sampler2D uTexture0;\n"
                                              "uniform sampler2D uTexture1;\n"
                                              "uniform sampler2D uTexture2;\n"
                                              "in vec3 lightDirection_tangentspace;\n"
                                              "in vec3 viewVector_tangentspace;\n"
                                              "in vec2 uv;\n"
                                              "in vec3 shDiffuse;\n"
                                              "out vec3 color;\n"
                                              "void main(void)\n"
                                              "{\n"
                                              "  vec3 normal_tagentspace = normalize(texture2D( uTexture1, uv ).rgb*2.0 - 1.0);\n"
                                              "  float diffuse = max(0.0,dot( normal_tagentspace,lightDirection_tangentspace ) )  * 0.5;\n"
                                              "  float ambient = 0.3;\n"
                                              "  vec3 reflection = normalize( -reflect( viewVector_tangentspace, normal_tagentspace));\n"
                                              " float specular = texture2D( uTexture2, uv ).r * pow(max(dot(reflection,lightDirection_tangentspace),0.0),100.0);\n"
                                              "  color = (shDiffuse + diffuse) * texture(uTexture0, uv ).rgb + vec3(specular,specular,specular);\n"
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


void ProjectLightToSH( vec3* sh, const vec3& lightIntensity, const vec3& lightDirection )
{
  static float fConst1 = 4.0/17.0;
  static float fConst2 = 8.0/17.0;
  static float fConst3 = 15.0/17.0;
  static float fConst4 = 5.0/68.0;
  static float fConst5 = 15.0/68.0;

  vec3 direction = lightDirection;
  direction.Normalize();

  sh[0] += lightIntensity * fConst1;
  sh[1] += lightIntensity * fConst2 * direction.x;
  sh[2] += lightIntensity * fConst2 * direction.y;
  sh[3] += lightIntensity * fConst2 * direction.z;
  sh[4] += lightIntensity * fConst3 * (direction.x * direction.z);
  sh[5] += lightIntensity * fConst3 * (direction.z * direction.y);
  sh[6] += lightIntensity * fConst3 * (direction.y * direction.x);
  sh[7] += lightIntensity * fConst4 * (3.0f * direction.z * direction.z - 1.0f);
  sh[8] += lightIntensity * fConst5 * (direction.x * direction.x - direction.y * direction.y);
}

void ProjectCubeMapToSH( vec3* sh, Image* cubemapImages )
{
  uvec2 imageSize = uvec2( cubemapImages[0].mWidth, cubemapImages[0].mHeight );
  unsigned int components = cubemapImages[0].mComponents;

  vec3 direction;
  f32 invWidth = 1.0f / float(imageSize.x);
  f32 invWidthBy2 = 2.0f / float(imageSize.x);
  f32 dw = 1.0f / (f32)(imageSize.x * imageSize.y);
  for( u32 face(0); face<6; ++face )
  {
    for( u32 y(0); y<imageSize.x; ++y )
    {
      for( u32 x(0); x<imageSize.y; ++x )
      {
        if( face == 0 )
        {
          direction = vec3( 1.0f,1.0f - ( invWidthBy2 * y + invWidth ), 1.0f - (invWidthBy2 * float(x) + invWidth) );
        }
        else if( face == 1 )
        {
          direction = vec3( -1.0f, 1.0f - (invWidthBy2 * float(y) + invWidth), -1.0f + (invWidthBy2 * float(x) + invWidth) );
        }
        else if( face == 2 )
        {
          direction = vec3( -1.0f + (invWidthBy2 * float(x) + invWidth), 1.0f, -1.0f + (invWidthBy2 * float(y) + invWidth) );
        }
        else if( face == 3 )
        {
          direction = vec3( -1.0f + (invWidthBy2 * float(x) + invWidth), -1.0f, 1.0f - (invWidthBy2 * float(y) + invWidth) );
        }
        else if( face == 4 )
        {
          direction = vec3( -1.0f + (invWidthBy2 * float(x) + invWidth), 1.0f - (invWidthBy2 * float(y) + invWidth), 1.0f );
        }
        else if( face == 5 )
        {
          direction = vec3( 1.0f - (invWidthBy2 * float(x) + invWidth), 1.0f - (invWidthBy2 * float(y) + invWidth), -1.0f );
        }

        u32 pixel = (x + y * imageSize.x) * components;
        vec3 color = vec3( cubemapImages[face].mData[pixel]/255.0f,
                                cubemapImages[face].mData[pixel+1]/255.0f,
                                cubemapImages[face].mData[pixel+2]/255.0f );

        ProjectLightToSH( sh, color*dw, direction );
      }
    }
  }
}


} //Unnamed namespace



class App : public Application
{
public:
  App()
:Application("Demo",500,500,4,4),
 mTxManager(1),
 mLightingEnv(1u),
 mCamera( 2.0f, vec2(0.0f,0.0f), 1.0f ),
 mMousePosition(0.0f,0.0f),
 mMouseButtonPressed(false)
{}

  ~App()
  {}

  void Init()
  {
    mProjection = ComputePerspectiveProjectionMatrix( DegreeToRadian(75.0f),(f32)mWindowSize.x / (f32)mWindowSize.y,1.0f,500.0f );
    ComputeSkyBoxTransform();

    //Create a texture
    Image image("data/R2D2_D.png");
    mColorTexture = mRenderManager.Add2DTexture( image );
    Image imageNormal("data/R2D2_N.png");
    mNormalTexture = mRenderManager.Add2DTexture( imageNormal,false );
    Image imageSpecular("data/R2D2_S.png");
    mSpecularTexture = mRenderManager.Add2DTexture( imageSpecular );

    //Create a shader
    mShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceDiffuse, (const u8**)gFragmentShaderSourceDiffuse);
    mQuadShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceTexture, (const u8**)gFragmentShaderSourceTexture);
    mAABBShader = mRenderManager.AddProgram((const u8**)gVertexShaderSourceAABB, (const u8**)gFragmentShaderSourceAABB);
    mSkyboxShader = mRenderManager.AddProgram((const u8**)gVertexShaderSkybox, (const u8**)gFragmentShaderSkybox);

    //Load meshes
    mMesh = mRenderManager.AddMeshFromFile( "data/R2D2.dae" );
    Mesh mesh = mRenderManager.GetMesh(mMesh);

    mTxId0 = mTxManager.CreateTransform();
    mTxManager.UpdatePosition( mTxId0, Negate(mesh.mAABB.mCenter) );
    mTxManager.Update();

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

    cubemapImages[0].LoadFromFile( "data/sky-box0/xpos.png", false);
    cubemapImages[1].LoadFromFile( "data/sky-box0/xneg.png", false);
    cubemapImages[2].LoadFromFile( "data/sky-box0/ypos.png", false);
    cubemapImages[3].LoadFromFile( "data/sky-box0/yneg.png", false);
    cubemapImages[4].LoadFromFile( "data/sky-box0/zpos.png", false);
    cubemapImages[5].LoadFromFile( "data/sky-box0/zneg.png", false);
    mCubeMap0 = mRenderManager.AddCubeTexture(&cubemapImages[0]);
    ProjectCubeMapToSH( &mSphericalHarmonics0[0], &cubemapImages[0] );

    cubemapImages[0].LoadFromFile( "data/sky-box1/posx.jpg", false);
    cubemapImages[1].LoadFromFile( "data/sky-box1/negx.jpg", false);
    cubemapImages[2].LoadFromFile( "data/sky-box1/posy.jpg", false);
    cubemapImages[3].LoadFromFile( "data/sky-box1/negy.jpg", false);
    cubemapImages[4].LoadFromFile( "data/sky-box1/posz.jpg", false);
    cubemapImages[5].LoadFromFile( "data/sky-box1/negz.jpg", false);
    mCubeMap1 = mRenderManager.AddCubeTexture(&cubemapImages[0]);
    ProjectCubeMapToSH( &mSphericalHarmonics1[0], &cubemapImages[0] );
  }

  void Render()
  {
    mRenderManager.BindFrameBuffer( mFbo );
    mRenderManager.ClearBuffers(COLOR_BUFFER | DEPTH_BUFFER );

    //Draw skybox
    mRenderManager.UseProgram( mSkyboxShader );
    if( mLightingEnv == 0 )
    {
      mRenderManager.BindCubeTexture( mCubeMap0, 0 );
    }
    else
    {
      mRenderManager.BindCubeTexture( mCubeMap1, 0 );
    }

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
    mRenderManager.Bind2DTexture( mNormalTexture, 1 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uTexture1"), 1 );
    mRenderManager.Bind2DTexture( mSpecularTexture, 2 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uTexture2"), 2 );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelViewProjection"), modelMatrix * mCamera.txInverse * mProjection );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModelView"), modelMatrix * mCamera.txInverse );
    mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"uModel"), modelMatrix );

    if( mLightingEnv == 0 )
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"shCoeff"), &mSphericalHarmonics0[0], 9 );
    }
    else
    {
      mRenderManager.SetUniform( mRenderManager.GetUniformLocation(mShader,"shCoeff"), &mSphericalHarmonics1[0], 9 );
    }

    mRenderManager.SetupMeshVertexFormat( mMesh );
    mRenderManager.DrawMesh( mMesh );

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
    mProjection = ComputePerspectiveProjectionMatrix( 1.5f,(f32)width / (f32)height,0.01f,500.0f );
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
          mCamera.Move( -.5f );
          ComputeSkyBoxTransform();
          break;
        }
        case KEY_DOWN:
        case 's':
        {
          mCamera.Move( .5f );
          ComputeSkyBoxTransform();
          break;
        }
        case 'x':
        {
          mLightingEnv = ( mLightingEnv + 1 ) % 2;
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

  MeshId      mMesh;
  TextureId   mColorTexture;
  TextureId   mNormalTexture;
  TextureId   mSpecularTexture;
  TextureId   mCubeMap0;
  TextureId   mCubeMap1;

  u32         mLightingEnv;
  vec3        mSphericalHarmonics0[9];
  vec3        mSphericalHarmonics1[9];
  ProgramId   mShader;

  MeshId      mQuad;
  ProgramId   mQuadShader;
  ProgramId   mAABBShader;
  ProgramId   mSkyboxShader;

  FBOId mFbo;
  TextureId mColorAttachment;
  TextureId mDepthStencilAttachment;

  OrbitingCamera mCamera;
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

