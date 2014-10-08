#ifndef __ndnrtc__addon__conference__info__
#define __ndnrtc__addon__conference__info__

#include <ndn-cpp/util/blob.hpp>
#include <ndn-cpp/ndn-cpp-config.h>

namespace conference_discovery
{
  class ConferenceInfo
  {
  public:
    ConferenceInfo()
    {
    
    }
    
    virtual Blob serialize(const ptr_lib::shared_ptr<ConferenceInfo> &info) = 0;
    virtual ptr_lib::shared_ptr<ConferenceInfo> deserialize(Blob srcBlob) = 0;
    
  protected:
    
  };
}

#endif