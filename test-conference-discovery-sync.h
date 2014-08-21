// Authored by Zhehao on Aug 19, 2014
// Sync-based conference discovery put off for now.

#ifndef __test_conference_discovery__
#define __test_conference_discovery__

#include <ndn-cpp/interest.hpp>
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/transport/tcp-transport.hpp>
#include <ndn-cpp/transport/udp-transport.hpp>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <ndn-cpp/security/identity/memory-identity-storage.hpp>
#include <ndn-cpp/security/identity/memory-private-key-storage.hpp>
#include <ndn-cpp/security/policy/no-verify-policy-manager.hpp>
#include <ndn-cpp/encoding/tlv-wire-format.hpp>
#include <ndn-cpp/encoding/binary-xml-wire-format.hpp>
#include <ndn-cpp/util/memory-content-cache.hpp>

#endif

using namespace std;

namespace conference_discovery
{
  // For now syncBasedDiscovery class contains a list of objects to be synchronized.
  // Probably will want to change into aggregation relationship at some point
  class SyncBasedDiscovery
  {
  public:
    SyncBasedDiscovery();
    ~SyncBasedDiscovery();
    
  private:
    
    //vector<> syncObjects;
  };
}