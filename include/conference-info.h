#ifndef __ndnrtc__addon__conference__info__
#define __ndnrtc__addon__conference__info__

#include <ndn-cpp/util/blob.hpp>
#include <ndn-cpp/ndn-cpp-config.h>

namespace conference_discovery
{
  // (TIMEOUTCOUNT + 1) timeouts in a row signifies conference dropped
  const int TIMEOUTCOUNT = 4;
  
  class ConferenceInfo
  {
  public:
    ConferenceInfo()
    {
      timeoutCount_ = 0;
      prefixId_ = -1;
      beingRemoved_ = false;
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
      if (timeoutCount_ ++ > TIMEOUTCOUNT) {
        return true;
      }
      else {
        return false;
      }
    }
    
    void resetTimeout() { timeoutCount_ = 0; }
    int getTimeoutCount() { return timeoutCount_; }
    
    std::string getConferenceName() { return conferenceName_; }
    void setConferenceName(std::string conferenceName) { conferenceName_ = conferenceName; }
    
    uint64_t getRegisteredPrefixId() { return prefixId_; }
    void setRegisteredPrefixId(uint64_t prefixId) { prefixId_ = prefixId; }
    
    bool getBeingRemoved() { return beingRemoved_; }
    void setBeingRemoved(bool beingRemoved) { beingRemoved_ = beingRemoved; }
  protected:
    int timeoutCount_;
    uint64_t prefixId_;
    std::string conferenceName_;
    bool beingRemoved_;
  };
}

#endif