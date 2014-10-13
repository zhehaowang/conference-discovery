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
    
    virtual ndn::Blob serialize(const ndn::ptr_lib::shared_ptr<ConferenceInfo> &info) = 0;
    virtual ndn::ptr_lib::shared_ptr<ConferenceInfo> deserialize(ndn::Blob srcBlob) = 0;
    
    /**
     * This part with aliases as names needs to be better designed...
     */
    virtual ndn::Blob 
    serializeName()
    { return ndn::Blob(); }
    
    virtual ndn::ptr_lib::shared_ptr<ConferenceInfo> 
    deserializeName()
    { return ndn::ptr_lib::shared_ptr<ConferenceInfo>(NULL); }
    
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
    
    std::string getConferenceName() { return conferenceName_; }
    void setConferenceName(std::string conferenceName) { conferenceName_ = conferenceName; }
  protected:
    int timeoutCount;
    std::string conferenceName_;
  };
}

#endif