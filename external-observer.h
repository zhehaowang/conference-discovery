// interface for external observer class

#ifndef __ndnrtc__addon__external__observer__
#define __ndnrtc__addon__external__observer__

namespace chrono_chat
{
  enum class MessageTypes
  {
    JOIN,
    LEAVE,
    CHAT
  };
  
  class ChatObserver
  {
  public:
    /**
     * The timestamp is considered as a double, which is the base type for ndn_Milliseconds in common.h
     */
    virtual void onStateChanged(MessageTypes type, const char *userName, const char *msg, double timestamp) = 0;
  };
}

namespace conference_discovery
{
  // enum class won't compile with C++03
  enum class MessageTypes
  {
    ADD,
    REMOVE,
    SET,
    START,
    STOP
  };
  
  class ConferenceDiscoveryObserver
  {
  public:
    /**
     * The timestamp is considered as a double, which is the base type for ndn_Milliseconds in common.h
     */
    virtual void onStateChanged(MessageTypes type, const char *msg, double timestamp) = 0;
  };
}

#endif