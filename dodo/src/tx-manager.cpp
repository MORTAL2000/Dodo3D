
#include <tx-manager.h>
#include <id.h>

#include <algorithm>    // std::sort

using namespace Dodo;

TxManager::TxManager()
:mComponentList(0),
 mParentId(0),
 mTxLocal(0),
 mTx(0)
{}

TxManager::TxManager( size_t size )
:mComponentList( new ComponentList<TxComponent>(size) ),
 mParentId( new TxId[size] ),
 mTxLocal( new mat4[size]),
 mTx( new mat4[size])
{
  mOrderedComponents.resize( size );
}

TxManager::~TxManager()
{
  delete   mComponentList;
  delete[] mParentId;
  delete[] mTxLocal;
  delete[] mTx;
}

TxId TxManager::CreateTransform( vec3 position, vec3 scale, quat orientation )
{
  return mComponentList->Add( TxComponent( position, scale, orientation ) );
}

TxId TxManager::CreateTransform(  const TxComponent& txComponent )
{
  return mComponentList->Add( txComponent );
}

bool TxManager::DestroyTransform( TxId id )
{
  return mComponentList->Remove( id );
}

bool TxManager::GetTransform( TxId id,  TxComponent* component) const
{
  TxComponent* c = mComponentList->GetElement(id);
  if( c )
  {
    *component = *c;
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::UpdateTransform( TxId id, const TxComponent& newData )
{
  TxComponent* component = mComponentList->GetElement(id);
  if( component )
  {
    *component = newData;
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::UpdatePosition( TxId id, vec3 position )
{
  TxComponent* component = mComponentList->GetElement(id);
  if( component )
  {
    component->mPosition = position;
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::UpdateScale( TxId id, vec3 scale )
{
  TxComponent* component = mComponentList->GetElement(id);
  if( component )
  {
    component->mScale = scale;
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::UpdateOrientation( TxId id, quat orientation)
{
  TxComponent* component = mComponentList->GetElement(id);
  if( component )
  {
    component->mOrientation = orientation;
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::SetParent( TxId id, TxId parentId )
{
  size_t index;
  if( mComponentList->GetIndexFromId( id, &index ) )
  {
    mParentId[index] = parentId;
    return true;
  }
  else
  {
    return false;
  }
}

TxId TxManager::GetParent( TxId id ) const
{
  size_t index;
  if( mComponentList->GetIndexFromId( id, &index ) )
  {
    return mParentId[index];
  }
  else
  {
    return INVALID_ID;
  }
}

bool TxManager::GetWorldTransform( TxId id, mat4* tx )
{
  size_t index;
  if( mComponentList->GetIndexFromId( id, &index ) )
  {
    *tx = mTx[index];
    return true;
  }
  else
  {
    return false;
  }
}

bool TxManager::GetLocalTransform( TxId id, mat4* tx )
{
  size_t index;
  if( mComponentList->GetIndexFromId( id, &index ) )
  {
    *tx = mTxLocal[index];
    return true;
  }
  else
  {
    return false;
  }
}

void TxManager::Update()
{
  //Calculate the level in the hierarchy of each component.
  //TODO: Use tasks?
  size_t parentIndex;
  TxId parentId;
  size_t componentCount( mComponentList->Size() );
  for( size_t i(0); i<componentCount; ++i )
  {
    mOrderedComponents[i].index = i;
    parentId = mParentId[i];
    while( mComponentList->GetIndexFromId( parentId, &parentIndex) )
    {
      mOrderedComponents[i].level++;
      parentId = mParentId[parentIndex];
    }
  }

  //Sort based on level to make sure we compute parent transform before children
  std::sort( mOrderedComponents.begin(), mOrderedComponents.begin()+componentCount );

  TxComponent* component;
  for( size_t i(0); i<componentCount; ++i )
  {
    const u32& index = mOrderedComponents[i].index;

    component = mComponentList->GetElementFromIndex( index );
    //@TODO: Only recompute this when node or some node higher in the hierarchy is dirty ( changed tx or hierarchy change )
    mTxLocal[index] = ComputeTransform( component->mPosition,component->mScale, component->mOrientation );
    mTx[index] = mTxLocal[index];
    if( mComponentList->GetIndexFromId(mParentId[index], &parentIndex ) )
    {
      mTx[index] = mTx[index]* mTx[parentIndex];
    }
  }
}

void TxManager::PrintTransforms()
{
  size_t componentCount( mComponentList->Size() );
  for( u32 i(0); i<componentCount; ++i )
  {
    std::cout<<mTx[i]<<std::endl;
  }
}
