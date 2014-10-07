// interface for external observer class

#ifndef __ndnrtc__addon__external__observer__
#define __ndnrtc__addon__external__observer__

namespace chrono_chat
{
  class ChatObserver
  {
  public:
    /**
     * The timestamp is considered as a double, which is the base type for ndn_Milliseconds in common.h
     */
    virtual void onStateChanged(const char *state, const char *userName, const char *msg, double timestamp) = 0;
  };
}

namespace conference_discovery
{  
  class ConferenceDiscoveryObserver
  {
  public:
    /**
     * The timestamp is considered as a double, which is the base type for ndn_Milliseconds in common.h
     */
    virtual void onStateChanged(const char *state, const char *msg, double timestamp) = 0;
  };
}

#endif