
#pragma once

#include <maths.h>
#include <hash-vector.h>
#include <component-list.h>

namespace Dodo
{

typedef Id  TxId;

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

  TxId CreateTransform( vec3 position = VEC3_ZERO, vec3 scale = VEC3_ONE, quat orientation = QUAT_UNIT );
  TxId CreateTransform(  const TxComponent& txComponent );
  bool DestroyTransform( TxId id );
  bool GetTransform( TxId id,  TxComponent* component) const;

  bool UpdateTransform( TxId id, const TxComponent& newData );
  bool UpdatePosition( TxId id, vec3 position );
  bool UpdateScale( TxId id, vec3 scale );
  bool UpdateOrientation( TxId id, quat orientation);

  bool SetParent( TxId id, Id parentId );
  TxId GetParent( TxId id ) const;

  bool GetWorldTransform( TxId id, mat4* tx );
  bool GetLocalTransform( TxId id, mat4* tx );

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

  TxId*                       mParentId;    ///< Id of the parent of the component
  mat4*                     mTxLocal;           ///< Local transform of the component
  mat4*                     mTx;                ///< World transform of the component

};

}
