
#pragma once
#include <maths.h>

namespace Dodo
{

struct FreeCamera
{
  FreeCamera( const vec3& position, const vec2& angle, f32 velocity )
  :mPosition(position),
   mAngle(angle),
   mVelocity(velocity)
  {
    Update();
  }

  void Move( f32 xAmount, f32 zAmount )
  {
    mPosition = mPosition + ( zAmount * mVelocity * mForward ) + ( xAmount * mVelocity * mRight );
    Update();
  }

  void Rotate( f32 angleX, f32 angleY )
  {
    mAngle.x =  mAngle.x - angleY;
    angleY = mAngle.y - angleX;
    if( angleY < M_PI_2 && angleY > -M_PI_2 )
    {
      mAngle.y = angleY;
    }

    Update();
  }

  void Update()
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

struct OrbitingCamera
{
  OrbitingCamera( const f32 offset, const vec2& angle, f32 velocity )
  :mOffset(offset),
   mAngle(angle),
   mVelocity(velocity)
  {
    Update();
  }

  void Move( f32 amount )
  {
    mOffset += amount;
    if( mOffset < 0.0f )
    {
      mOffset = 0.0f;
    }

    Update();
  }

  void Rotate( f32 angleY, f32 angleZ )
  {
    mAngle.x =  mAngle.x - angleY;
    angleY = mAngle.y - angleZ;
    if( angleY < M_PI_2 && angleY > -M_PI_2 )
    {
      mAngle.y = angleY;
    }

    Update();
  }

  void Update()
  {
    quat orientation =  QuaternionFromAxisAngle( vec3(0.0f,1.0f,0.0f), mAngle.x ) *
        QuaternionFromAxisAngle( vec3(1.0f,0.0f,0.0f), mAngle.y );

    mForward = Dodo::Rotate( vec3(0.0f,0.0f,1.0f), orientation );
    mRight = Cross( mForward, vec3(0.0f,1.0f,0.0f) );
    tx = ComputeTransform(vec3(0.0f,0.0f,mOffset), VEC3_ONE, QUAT_UNIT) * ComputeTransform( VEC3_ZERO, VEC3_ONE, orientation );

    ComputeInverse( tx, txInverse );
  }

  mat4 tx;
  mat4 txInverse;

  f32 mOffset;
  vec3 mForward;
  vec3 mRight;
  vec2 mAngle;
  f32  mVelocity; //Units per second
};


};
