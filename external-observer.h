// interface for external observer class

#ifndef __ndnrtc__addon__external__observer__
#define __ndnrtc__addon__external__observer__

namespace chrono_chat
{
  class ExternalObserver
  {
  public:
    virtual void onStateChanged(const char *state, const char *args) = 0;
  }; 
}

#endif