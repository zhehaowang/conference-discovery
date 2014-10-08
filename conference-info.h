#ifndef __ndnrtc__addon__conference__info__
#define __ndnrtc__addon__conference__info__

#include <ndn-cpp/util/blob.hpp>
#include <ndn-cpp/ndn-cpp-config.h>

namespace conference_discovery
{
  // (TIMEOUTCOUNT + 1) timeouts in a row signifies conference dropped
  const int TIMEOUTCOUNT = 2;
  
  class ConferenceInfo
  {
  public:
    ConferenceInfo()
    {
      timeoutCount = 0;
    }
    
    virtual Blob serialize(const ptr_lib::shared_ptr<ConferenceInfo> &info) = 0;
    virtual ptr_lib::shared_ptr<ConferenceInfo> deserialize(Blob srcBlob) = 0;
    
    bool incrementTimeout()
    {
      if (timeoutCount ++ > TIMEOUTCOUNT) {
        return true;
      }
      else {
        return false;
      }
    }
    
    void resetTimeout() { timeoutCount = 0; }
    int getTimeoutCount() { return timeoutCount; }
  protected:
    int timeoutCount;
  };
}

#endif