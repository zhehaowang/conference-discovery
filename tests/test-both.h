#ifndef __ndnrtc__addon__test__
#define __ndnrtc__addon__test__

#include "external-observer.h"
#include "chrono-chat.h"
#include "conference-discovery-sync.h"

#include <ndn-cpp/util/blob.hpp>

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
    
    void onStateChanged(chrono_chat::MessageTypes type, const char *prefix, const char *userName, const char *msg, double timestamp)
    {
      cout << timestamp << "\t" << userName << " : " << msg << endl;
    }
  private:
  
  };
  
  class TestConferenceDiscoveryObserver : public ConferenceDiscoveryObserver
  {
  public:
    TestConferenceDiscoveryObserver()
    {
    
    }
    
    void onStateChanged(conference_discovery::MessageTypes type, const char *msg, double timestamp)
    {
      cout << timestamp << "\t" << msg << endl;
    }
  };
  
  class ConferenceDescription : public ConferenceInfo
  {
  public:
    ConferenceDescription()
    {
      description_ = "No description";
    }
    
    // TODO: This does not feel right...
    virtual Blob
    serialize(const ptr_lib::shared_ptr<ConferenceInfo> &description)
    {
      string content = (ptr_lib::dynamic_pointer_cast<ConferenceDescription>(description))->getDescription();
      return Blob((const uint8_t *)(&content[0]), content.size());
    }
    
    virtual ptr_lib::shared_ptr<ConferenceInfo>
    deserialize(Blob srcBlob)
    {
      //cout << "deserialize from blob not implemented." << endl;
      string content = "";
      for (size_t i = 0; i < srcBlob.size(); ++i) {
        content += (*srcBlob)[i];
      }
      
      ConferenceDescription cd;
      cd.setDescription(content);
      return ptr_lib::make_shared<ConferenceDescription>(cd);
    }
    
    void
    setDescription(string description)
    {
      description_ = description;
    }
    
    string
    getDescription()
    {
      return description_;
    }
  private:
    string description_;
  };
}

#endif