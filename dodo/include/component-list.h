

#pragma once

#include <hash-vector.h>
#include <types.h>

namespace Dodo
{

template <typename T>
struct ComponentList
{
  ComponentList()
  :mHash(0),
   mData(0),
   mId(0),
   mCapacity(0),
   mSize(0)
  {
  }

  ComponentList( size_t maxElementCount )
  :mHash( new HashVector<size_t>(maxElementCount) ),
   mData( new T[maxElementCount] ),
   mId( new Id[maxElementCount] ),
   mCapacity(maxElementCount),
   mSize(0)
  {
  }

  ~ComponentList()
  {
    delete[] mData;
    delete[] mId;
  }

  Id Add( const T& element )
  {
    if( mSize < mCapacity )
    {
      //Add a new element to the end
      Id id = mHash->Add( mSize );
      mData[mSize] = element;
      mId[mSize] = id;
      ++mSize;

      return id;
    }
    else
    {
      return INVALID_ID;
    }
  }

  bool Remove( Id id )
  {
    size_t index;
    if( mHash->Get( id, &index ) )
    {
      if( index < mSize-1 )
      {
        mData[index] = mData[mSize-1];
        mHash->Set( mId[index], index );
      }

      mSize--;
      mHash->Remove( id );
      return true;
    }

    return true;
  }

  T* GetElement( Id id )
  {
    size_t index;
    if( mHash->Get( id, &index ) )
    {
      return &(mData[index]);
    }

    return 0;
  }

  Id GetIdFromIndex( size_t index )
  {
    return mId[index];
  }

  bool GetIndexFromId( Id id, size_t* index )
  {
    return mHash->Get( id, index );
  }

  T* GetElementFromIndex( size_t index )
  {
    return &(mData[index]);
  }


  size_t Size()
  {
    return mSize;
  }

  HashVector<size_t>* mHash;      //Id -> Index
  T*                  mData;
  Id*                 mId; ///< Id of the component

  size_t              mCapacity;
  size_t              mSize;

};

}
