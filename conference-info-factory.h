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
    ConferenceInfoFactory(ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo)
    {
      conferenceInfo_ = conferenceInfo;
    }
    
    Blob 
    serialize(ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo)
    {
      try {
        return conferenceInfo_->serialize(conferenceInfo);
      }
      catch (std::exception& e) {
        cout << e.what() << endl;
      }
      return Blob();
    }
    
    ptr_lib::shared_ptr<ConferenceInfo> 
    deserialize(Blob srcBlob)
    {
      try {
        return conferenceInfo_->deserialize(srcBlob);
      }
      catch (std::exception& e) {
        cout << e.what() << endl;
      }
      return NULL;
    }
  protected:
    ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo_;
  };
}

#endif