#ifndef __ndnrtc__addon__test__
#define __ndnrtc__addon__test__

#include "external-observer.h"
#include "test-chrono-chat.h"
#include "test-conference-discovery-sync.h"

using namespace ndn;
using namespace chrono_chat;
using namespace conference_discovery;

namespace test
{
  class TestChatObserver : public ChatObserver
  {
  public:
    TestChatObserver()
    {
    
    }
    
    void onStateChanged(const char *state, const char *userName, const char *msg, double timestamp)
    {
      cout << state << " " << timestamp << "\t" << userName << " : " << msg << endl;
    }
  private:
  
  };
  
  class TestConferenceDiscoveryObserver : public ConferenceDiscoveryObserver
  {
  public:
    TestConferenceDiscoveryObserver()
    {
    
    }
    
    void onStateChanged(const char *state, const char *msg, double timestamp)
    {
      cout << state << " " << timestamp << "\t" << msg << endl;
    }
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