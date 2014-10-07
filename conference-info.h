#ifndef __ndnrtc__addon__external__observer__
#define __ndnrtc__addon__external__observer__

namespace conference_discovery
{
  class ConferenceInfo
  {
  public:
    ConferenceInfo()
    {
    
    }
    
    virtual std::string serialize() = 0;
    virtual void deserialize(std::string srcString) = 0;
    
    std::string getString()
    {
      return string_;
    };
    
  private:
    std::string string_;
    
  }
}

#endif