
#pragma once

#include <vector>
#include <id.h>

namespace Dodo
{

template <typename T>
struct HashVector
{
  HashVector()
  :mFirstFreeIndex(0)
  {}

  HashVector( size_t size )
  :mFirstFreeIndex(0)
  {
    mData.resize(size);
    mGeneration.resize(size);

    //Initialize free-list
    for( size_t i(0); i<size; ++i )
    {
      mData[i] = i+1;
      mGeneration[i] = 0;
    }
  }

  Id Add( const T& data )
  {
    if( mData.empty() || mFirstFreeIndex == mData.size() )
    {
      //Make more room
      size_t size = mData.size();
      mData.resize( size+1 );
      mData[size] = size+1;
      mGeneration.resize( size+1 );
      mGeneration[size] = 0;
      mFirstFreeIndex = size;
    }

    //Update first free index
    size_t index = mFirstFreeIndex;
    mFirstFreeIndex = mData[mFirstFreeIndex];

    mData[index] = data;
    return Id(index,mGeneration[index]);
  }

  void Remove( Id id )
  {
    mData[id.mIndex] = mFirstFreeIndex;
    ++mGeneration[id.mIndex];
    mFirstFreeIndex = id.mIndex;
  }

  bool Get( Id id, T* result) const
  {
    if( id.mIndex < mData.size() && id.mGeneration == mGeneration[id.mIndex] )
    {
      *result = mData[id.mIndex];
      return true;
    }

    return false;
  }

  bool Set( Id id, T& value )
  {
    if( id.mGeneration == mGeneration[id.mIndex] )
    {
      mData[id.mIndex] = value;
      return true;
    }

    return false;
  }

private:
  std::vector<T>        mData;            //Free list (sparse)
  std::vector<unsigned> mGeneration;
  size_t                mFirstFreeIndex;
};

}	//namespace
