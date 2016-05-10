
#include "ray-tracer.h"
#include <cstdlib>

using namespace Dodo;

/**
 * Camera
 */
RayTracer::Camera::Camera(f32 verticalFov, f32 aperture, f32 focalDistance, u32 imageWidth, u32 imageHeight )
: mVerticalFov(verticalFov),
  mAperture(aperture),
  mFocalDistance(focalDistance),
  mImageWidth( imageWidth ),
  mImageHeight( imageHeight ),
  mPixelWidth( 1.0f / (f32(imageWidth-1)) ),
  mPixelHeight( 1.0f / (f32(imageHeight-1)) ),
  mAspectRatio( (f32)imageWidth / (f32)imageHeight),
  mImagePlaneHeight( tan( DegreeToRadian(mVerticalFov) * 0.5f ) ),
  mImagePlaneWidth( mAspectRatio * mImagePlaneHeight ),
  mFrameCount(0),
  mFreeCamera( vec3(0.0f,0.0f,0.0f), vec2(0.0f,0.0f), 1.0f )
{}

const mat4& RayTracer::Camera::GetTx()
{
  return mFreeCamera.tx;
}


void RayTracer::Camera::MoveCamera( f32 xAmount, f32 zAmount)
{
  mFreeCamera.Move( xAmount, zAmount );
  mFrameCount = 0;
}

void RayTracer::Camera::RotateCamera( f32 angleX, f32 angleY)
{
  mFreeCamera.Rotate( angleX, angleY );
  mFrameCount = 0;
}

void RayTracer::Camera::GenerateRayWithDOF( u32 pixelX, u32 pixelY, vec3* rayOrigin, vec3* rayDirection )
{
  //Random point inside the pixel
  f32 u = ( pixelY + drand48() ) * mPixelWidth - 0.5f;
  f32 v = ( pixelX + drand48() ) * mPixelHeight - 0.5f;

  //Compute intersection with focal plane in world space
  vec4 focalPlaneIntersection = vec4( u * mImagePlaneWidth * mFocalDistance, v * mImagePlaneHeight * mFocalDistance, -mFocalDistance, 1.0f ) * GetTx();

  //Get random point in the lens in world space
  vec4 lensRadomPoint = vec4( u * mImagePlaneWidth*(2.0f*drand48()-1.0f) * mAperture, v * mImagePlaneHeight*(2.0f*drand48()-1.0f) * mAperture, 0.0f, 1.0f ) * GetTx();

  *rayDirection = Normalize( focalPlaneIntersection.xyz() - lensRadomPoint.xyz() );
  *rayOrigin = lensRadomPoint.xyz();

}

u32 RayTracer::Camera::IncrementFrameCount()
{
  return ++mFrameCount;
}

/**
 * Scene
 */
void RayTracer::Scene::AddMaterial( const Scene::Material& m )
{
  mMaterial.push_back( m );
}

void RayTracer::Scene::AddSphere( const Scene::Sphere& s )
{
  mObjects.push_back( s );
}

bool RayTracer::Scene::Intersect(const vec3& rayOrigin, const vec3& rayDirection, f32 tMin, f32 tMax, Hit& hit)
{
  bool intersect(false);
  f32 closestT = tMax;
  for( u32 i(0); i<mObjects.size(); ++i )
  {
    if( IntersectSphere( i, rayOrigin, rayDirection, tMin, closestT, hit ) )
    {
      closestT = hit.t;
      intersect = true;
    }
  }

  return intersect;
}

bool RayTracer::Scene::IntersectSphere( u32 sphereIndex, const vec3& rayOrigin, const vec3& rayDirection, f32 tMin, f32 tMax, Hit& hit )
{
  const vec3& sphereCenter = mObjects[sphereIndex].center;
  f32 sphereRadius = mObjects[sphereIndex].radius;

  vec3 rayOriginToCentre = rayOrigin - sphereCenter;

  if( Dot( rayDirection, Normalize(rayOriginToCentre) ) > 0.0f )
    return false;

  f32 a = Dot( rayDirection, rayDirection );
  f32 b = 2.0f * Dot(rayOriginToCentre, rayDirection );
  f32 c = Dot(rayOriginToCentre,rayOriginToCentre) - sphereRadius*sphereRadius;

  f32 discriminant = b*b - 4.0*a*c;
  if( discriminant > 0.0f )
  {
    f32 t = (-b - sqrtf( discriminant )) / (2.0f*a);
    if( t < tMax && t > tMin )
    {
      hit.t = t;
      hit.position = rayOrigin + rayDirection * t;
      hit.normal = ( hit.position - sphereCenter) / sphereRadius;
      hit.material = mObjects[sphereIndex].material;
      return true;
    }
  }

  return false;
}

vec3 RayTracer::Scene::GetColor(vec3 rayOrigin,  vec3 rayDirection, u32 iterationMax, u32& iteration )
{
  vec3 lightDirection = Normalize( vec3(1.0f,1.0f,0.3f) );
  vec3 color;
  Hit hit;
  if( iteration < iterationMax && Intersect(rayOrigin, rayDirection, 0.01f, 1000.0f, hit ) )
  {
    vec3 attenuation;
    vec3 bounceDirection;

    if( mMaterial[hit.material].Sample( hit.position, hit.normal, rayDirection, bounceDirection, attenuation ) )
    {
      return attenuation * GetColor( hit.position, bounceDirection, iterationMax, ++iteration );
    }
    else
    {
      return vec3(0.0f,0.0f,0.0f);
    }
  }
  else
  {
    f32 t = rayDirection.y * 0.5f + 0.5f;
    return Lerp( vec3(0.0f,0.0f,0.0f),vec3(0.6f,0.6f,1.0f), t );
  }

  return color;
}

/**
 * Material
 */
bool RayTracer::Scene::Material::Sample( const vec3& position, const vec3& normal, const vec3& rayDirection, vec3& newDirection, vec3& attenuation )
{
  f32 v = drand48();

  if( v < metalness )
  {
    vec3 reflected = Reflect( rayDirection, normal );
    newDirection = reflected + roughness*GetRandomPointInUnitSphere();
    attenuation = albedo;
    return Dot( newDirection, normal ) > 0.0f;
  }
  else
  {
    newDirection = GetSampleBiased(normal,1.0);
    attenuation = albedo * Dot( normal, newDirection ) ;
    return true;
  }
}

vec3 RayTracer::Scene::Material::GetSampleBiased( const vec3& dir, float power )
{
  vec3 o1 = Normalize(Ortho(dir));
  vec3 o2 = Normalize(Cross(dir, o1));
  vec2 r = vec2(drand48(),drand48());
  r.x=r.x*2.0* M_PI;
  r.y=pow(r.y,1.0/(power+1.0));
  float oneminus = sqrt(1.0-r.y*r.y);

  f32 a = cosf(r.x)*oneminus;
  f32 b = sin(r.x)*oneminus;
  return a*o1 + b*o2 + r.y*dir;
}

vec3 RayTracer::Scene::Material::GetRandomPointInUnitSphere()
{
  vec3 point(0.0f,0.0f,0.0f);
  do
  {
    point = 2.0f * vec3(drand48(),drand48(),drand48()) - vec3(1.0f,1.0f,1.0f);
  }while( LenghtSquared( point ) >= 1.0f );

  return point;
}

vec3 RayTracer::Scene::Material::Ortho(vec3 v)
{
  return abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0)  : vec3(0.0, -v.z, v.y);
}

/**
 * Renderer
 */
RayTracer::Renderer::Renderer()
:mScene(0),
 mCamera(0),
 mResolution(),
 mPool(8),
 mTasks(0),
 mTaskCount(0),
 mImageData(0),
 mAccumulationBuffer(0)
{}

RayTracer::Renderer::~Renderer()
{
  delete[] mImageData;
  delete[] mAccumulationBuffer;
  mPool.Exit();
  delete[] mTasks;
}

void RayTracer::Renderer::Initialize( uvec2 resolution, uvec2 tileSize, Camera* camera, Scene* scene )
{
  mCamera = camera;
  mScene = scene;

  mResolution = resolution;

  u32 bufferSize(resolution.x*resolution.y*3);
  mImageData = new u8[bufferSize];
  mAccumulationBuffer = new vec3[resolution.x*resolution.y];

  //Setup tasks
  u32 horizontalTiles = mResolution.x / tileSize.x;
  u32 verticalTiles = mResolution.y / tileSize.y;
  mTaskCount = horizontalTiles * verticalTiles;
  mTasks = new Tile[mTaskCount];
  for( u32 i(0); i<horizontalTiles; ++i )
  {
    for( u32 j(0); j<verticalTiles; ++j )
    {
      u32 taskIndex = i*horizontalTiles + j;
      mTasks[taskIndex].mImageResolution = resolution;
      mTasks[taskIndex].mTileSize = tileSize;
      mTasks[taskIndex].mTile = uvec2( j*tileSize.x, i*tileSize.y );
      mTasks[taskIndex].mScene = mScene;
      mTasks[taskIndex].mCamera = mCamera;
      mTasks[taskIndex].mAccumulationBuffer = mAccumulationBuffer;
    }
  }
}

void RayTracer::Renderer::Render()
{
  u32 frameCount = mCamera->IncrementFrameCount();
  if( frameCount == 1u )
  {
    memset( mAccumulationBuffer,0.0f, mResolution.x * mResolution.y*sizeof(vec3));
  }

  for( u32 i(0); i<mTaskCount; ++i )
  {
    mPool.AddTask( &mTasks[i] );
  }
  mPool.WaitForCompletion();

  u32 pixelCount = mResolution.x*mResolution.y;
  u32 count(0);
  vec3 color;
  for( u32 i(0); i<pixelCount; ++i )
  {
    color = mAccumulationBuffer[i] * ( 1.0f / frameCount );
    mImageData[count++] = u8( 255.0f * sqrtf(color.x) );
    mImageData[count++] = u8( 255.0f * sqrtf(color.y) );
    mImageData[count++] = u8( 255.0f * sqrtf(color.z) );
  }
}

u8* RayTracer::Renderer::GetImage()
{
  return mImageData;
}

void RayTracer::Renderer::Tile::Run()
{
  vec3 rayDirection;
  vec3 rayOrigin;

  for( u32 row(mTile.y); row<mTile.y+mTileSize.y; ++row )
  {
    for( u32 column(mTile.x); column<mTile.x+mTileSize.x; ++column )
    {
      mCamera->GenerateRayWithDOF( row, column, &rayOrigin, &rayDirection );

      u32 iteration(0);
      u32 pixelIndex = row*mImageResolution.x + column;
      mAccumulationBuffer[pixelIndex] += mScene->GetColor( rayOrigin, rayDirection, 4, iteration );
    }
  }
}
