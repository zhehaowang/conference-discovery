#ifndef __ndnrtc__addon__conference__info__factory__
#define __ndnrtc__addon__conference__info__factory__

#include <ndn-cpp/ndn-cpp-config.h>
#include <ndn-cpp/util/blob.hpp>
#include <exception>

#include "conference-info.h"

namespace conference_discovery
{
  class ConferenceInfoFactory
  {
  public:
    ConferenceInfoFactory(ndn::ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo)
    {
      conferenceInfo_ = conferenceInfo;
    }
    
    ndn::Blob 
    serialize(ndn::ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo)
    {
      try {
        return conferenceInfo_->serialize(conferenceInfo);
      }
      catch (std::exception& e) {
        // serialize must be defined, and (probably) shouldn't give an empty string;
        std::cerr << e.what() << std::endl;
        throw;
      }
      return ndn::Blob();
    }
    
    ndn::ptr_lib::shared_ptr<ConferenceInfo> 
    deserialize(ndn::Blob srcBlob)
    {
      try {
        return conferenceInfo_->deserialize(srcBlob);
      }
      catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
      }
      return ndn::ptr_lib::shared_ptr<ConferenceInfo>(NULL);
    }
  protected:
    ndn::ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo_;
  };
}

#endif