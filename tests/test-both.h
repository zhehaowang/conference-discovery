#ifndef __ndnrtc__addon__test__
#define __ndnrtc__addon__test__

#include "external-observer.h"
#include "chrono-chat.h"
#include "entity-discovery.h"

#include <ndn-cpp/util/blob.hpp>

using namespace ndn;
using namespace std;
using namespace chrono_chat;
using namespace entity_discovery;

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
  
  class TestConferenceDiscoveryObserver : public IDiscoveryObserver
  {
  public:
    TestConferenceDiscoveryObserver()
    {
    
    }
    
    void onStateChanged(entity_discovery::MessageTypes type, const char *msg, double timestamp)
    {
      cout << "onStateChanged: " << (int)type << "\t" << msg << "\t" <<  timestamp << "\t" << msg << endl;
    }
  };
  
  class ConferenceDescription : public EntityInfoBase
  {
  public:
    ConferenceDescription()
    {
      description_ = "No description";
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

  class ConferenceDescriptionSerializer : public IEntitySerializer 
  {
  public:
    virtual Blob
    serialize(const ptr_lib::shared_ptr<EntityInfoBase> &description)
    {
      string content = (ptr_lib::dynamic_pointer_cast<ConferenceDescription>(description))->getDescription();
      return Blob((const uint8_t *)(&content[0]), content.size());
    }
    
    virtual ptr_lib::shared_ptr<EntityInfoBase>
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
  };
}

#endif