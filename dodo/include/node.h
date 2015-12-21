
#pragma once

#include <component.h>
#include <tx-manager.h>
#include <id.h>


namespace Dodo
{

//struct Message
//{
//  Node* mSender;
//
//};

struct Node
{
  Node()
  {
    for( unsigned int i(0); i<Component::COUNT; ++i )
    {
      mComponent[i] = INVALID_ID;
    }
  }

  bool HasComponent( Component::Type type )
  {
    return mComponent[type] != INVALID_ID;
  }

  void AddComponent( Component::Type type, Id id )
  {
    mComponent[type] = id;
  }

  void RemoveComponent( Component::Type type )
  {
    mComponent[type] = INVALID_ID;
  }

  Id GetComponentId( Component::Type type )
  {
    return mComponent[type];

  }
  //  void SendMessage( Message* m )
  //  {
  //     for( int i(0); i<Component::COUNT; ++i )
  //     {
  //         mComponent[i].Recieve(m);
  //     }
  //  }


  Id mComponent[Component::COUNT];
};

}
