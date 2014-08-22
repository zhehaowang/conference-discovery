#include "test-conference-discovery-sync.h"

using namespace ndn;
using namespace std;

using namespace conference_discovery;

void SyncBasedDiscovery::onData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  
}

void SyncBasedDiscovery::onTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  cout << "Sync interest times out." << endl;
}

void SyncBasedDiscovery::onInterest
  (const ptr_lib::shared_ptr<const Name>& prefix,
   const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
   uint64_t registerPrefixId)
{
  // memoryContentCache should be able to satisfy interests marked with "ANSWER_NO_CONTENT_STORE",
  // Will it cause potential problems?
  string syncDigest = interest->getName().get
    (broadcastPrefix_.size()).toEscapedString();
    
  if (syncDigest != currentDigest_) {
    // the syncDigest differs from the local knowledge, reply with local knowledge
    // It could potentially cause one name corresponds with different data in different locations,
    // Same as publishing two conferences simultaneously: such errors should be correctible
    // later steps
    
  }
  else if (syncDigest != newComerDigest_) {
    // Store this steady-state (outstanding) interest in application PIT, unless neither the sender
    // and receiver knows anything
    
    
  }
}
    
void SyncBasedDiscovery::onRegisterFailed
  (const ptr_lib::shared_ptr<const Name>& prefix)
{
  
}    