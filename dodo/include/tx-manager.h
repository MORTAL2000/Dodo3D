
#pragma once

#include <maths.h>
#include <hash-vector.h>
#include <component-list.h>

namespace Dodo
{

struct TxComponent
{
  TxComponent()
  :mPosition(VEC3_ZERO),
   mScale(VEC3_ONE),
   mOrientation(QUAT_UNIT)
  {}

  TxComponent( vec3 position, vec3 scale, quat q):mPosition(position),mScale(scale),mOrientation(q){}

  vec3 mPosition;
  vec3 mScale;
  quat mOrientation;
};


struct TxManager
{
  TxManager();
  TxManager( size_t size );
  ~TxManager();

  Id CreateTransform( vec3 position = VEC3_ZERO, vec3 scale = VEC3_ONE, quat orientation = QUAT_UNIT );
  Id CreateTransform(  const TxComponent& txComponent );
  bool DestroyTransform( Id id );
  bool GetTransform( Id id,  TxComponent* component) const;

  bool UpdateTransform( Id id, const TxComponent& newData );
  bool UpdatePosition( Id id, vec3 position );
  bool UpdateScale( Id id, vec3 scale );
  bool UpdateOrientation( Id id, quat orientation);

  bool SetParent( Id id, Id parentId );
  Id GetParent( Id id ) const;

  bool GetWorldTransform( Id id, mat4* tx );
  bool GetLocalTransform( Id id, mat4* tx );

  void Update();

  void PrintTransforms();

private:

  /*
   * Helper structure to order items
   */
  struct SOrderItem
  {
    u32 index;
    s32 level;

    SOrderItem():index(0),level(0){}
    SOrderItem( u32 i, s32 l ):index(i),level(l){}
    bool operator<(const SOrderItem& item) const {return level < item.level;}
  };


  ComponentList<TxComponent>*  mComponentList;
  std::vector<SOrderItem>   mOrderedComponents; ///< List of components ordered by its level in hierarchy

  Id*                       mParentId;    ///< Id of the parent of the component
  mat4*                     mTxLocal;           ///< Local transform of the component
  mat4*                     mTx;                ///< World transform of the component

};

}
