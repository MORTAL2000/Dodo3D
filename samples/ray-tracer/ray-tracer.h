
#pragma once

#include <task.h>
#include <tx-manager.h>
#include <maths.h>
#include <camera.h>

using namespace Dodo;

namespace RayTracer
{

struct Camera
{
  Camera(f32 verticalFov, f32 aperture, f32 focalDistance, u32 imageWidth, u32 imageHeight );
  const mat4& GetTx();
  void MoveCamera( f32 xAmount, f32 zAmount);
  void RotateCamera( f32 angleX, f32 angleY);
  void GenerateRayWithDOF( u32 pixelX, u32 pixelY, vec3* rayOrigin, vec3* rayDirection );

  u32 IncrementFrameCount();

private:
  f32 mVerticalFov;
  f32 mAperture;
  f32 mFocalDistance;
  u32 mImageWidth;
  u32 mImageHeight;
  f32 mPixelWidth;
  f32 mPixelHeight;
  f32 mAspectRatio;
  f32 mImagePlaneHeight;
  f32 mImagePlaneWidth;
  u32 mFrameCount;
  FreeCamera mFreeCamera;

};

struct Scene
{
public:

  struct Sphere
  {
    vec3 center;
    f32 radius;
    u32 material;
  };

  struct Material
  {
    vec3 albedo;
    float roughness;
    float metalness;

    bool Sample( const vec3& position, const vec3& normal, const vec3& rayDirection, vec3& newDirection, vec3& attenuation );

  private:
    vec3 GetSampleBiased( const vec3& dir, float power );
    vec3 GetRandomPointInUnitSphere();
    vec3 Ortho(vec3 v);
  };

  void AddMaterial( const Scene::Material& m );
  void AddSphere( const Scene::Sphere& s );
  vec3 GetColor(vec3 rayOrigin,  vec3 rayDirection, u32 iterationMax, u32& iteration );

  std::vector<Sphere> mObjects;
  std::vector<Material> mMaterial;

private:

  struct Hit
  {
    Hit()
    :t(0.0f),
     position(0.0f,0.0f,0.0f),
     normal(0.0f,0.0f,0.0f),
     material(0)
    {}

    f32 t;
    vec3 position;
    vec3 normal;
    u32  material;
  };

  bool Intersect(const vec3& rayOrigin, const vec3& rayDirection, f32 tMin, f32 tMax, Hit& hit);
  bool IntersectSphere( u32 sphereIndex, const vec3& rayOrigin, const vec3& rayDirection, f32 tMin, f32 tMax, Hit& hit );
};

struct Renderer
{
public:
  Renderer();
  void Initialize( uvec2 resolution, uvec2 tileSize, Camera* camera, Scene* scene );
  void Render();
  ~Renderer();

  u8* GetImage();

private:
	
	struct AccumulationBuffer
	{
		AccumulationBuffer()
		:mBuffer(0),
		 mResolution(uvec2(0u,0u))
		{}

		~AccumulationBuffer()
		{
			delete[] mBuffer;
		}

		void Accumulate( const vec3& color, u32 x, u32 y );
		void Reset();
		
		vec3* mBuffer;
		uvec2 mResolution;		
	};

  struct Tile : public Dodo::ITask
  {
    void Run();

    AccumulationBuffer* mAccumulationBuffer;
    uvec2 mTileOrigin;
    uvec2 mTileSize;    
    Scene* mScene;
    Camera* mCamera;
  };

  Scene* mScene;
  Camera* mCamera;
  uvec2 mResolution;
  ThreadPool mPool;
  Tile* mTasks;
  u32 mTaskCount;
  u8* mImageData;
  AccumulationBuffer mAccumulationBuffer;
};

}
