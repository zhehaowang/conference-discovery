#ifndef __ndnrtc__addon__test__
#define __ndnrtc__addon__test__

#include "external-observer.h"
#include "test-chrono-chat.h"
#include "test-conference-discovery-sync.h"

using namespace ndn;

namespace chrono_chat
{
  class Observer : public ExternalObserver
  {
  public:
    Observer()
    {
    
    }
    void onStateChanged(const char *state, const char *args)
    {
      cout << "State changed: " << state << args << endl;
    }
  private:
  
  };
  
  class Test
  {
  public:
    Test()
    {
    
    }
    
    ~Test()
    {
    
    }
    
    int startChronoChat();
    int stopChronoChat();
    
    int startDiscovery();
    int stopDiscovery();
  private:
    Face face_;
    KeyChain keyChain_;
    Name certificateName_;
  };
}

#endif