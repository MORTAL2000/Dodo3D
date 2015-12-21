
#pragma once

#include <id.h>
#include <component-list.h>
#include <tx-manager.h>
#include <node.h>


namespace Dodo
{

struct SceneGraph
{
  SceneGraph()
  :mNode(0),
   mTxManager(0)
  {
  }

  SceneGraph( size_t maxNodes )
  :mNode( new ComponentList<Node>(maxNodes) ),
   mTxManager( new TxManager( maxNodes))
  {
  }

  ~SceneGraph()
  {
    delete mNode;
    delete mTxManager;
  }

  Id AddNode()
  {
    Id nodeId = mNode->Add(Node());
    return nodeId;
  }

  bool DestroyNode( Id id )
  {
    return mNode->Remove(id);
  }

  //Id AddTransformComponent( Id nodeId, const TxComponent& txComponent )
  //{
  //}

  ComponentList<Node>*  mNode;
  TxManager*        mTxManager;
};


}
