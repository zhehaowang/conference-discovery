#include "test-conference-discovery-sync.h"
#include <sys/time.h>
#include <openssl/ssl.h>

using namespace ndn;
using namespace std;

using namespace conference_discovery;

void 
SyncBasedDiscovery::onData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  cout << "Data " << data->getName().toUri() << " received." << endl;
  
  string content;
  for (size_t i = 0; i < data->getContent().size(); ++i)
    content += (*data->getContent())[i];
  
  vector<std::string> objects = SyncBasedDiscovery::stringToObjects(content);
  
  cout << "Data content contained " << content << endl;
  
  // Finding vector differences by using existing function, 
  // which requires both vectors to be sorted.
  std::sort(objects.begin(), objects.end());
  std::sort(objects_.begin(), objects_.end());

  std::vector<std::string> setDifferences;

  std::set_symmetric_difference(
    objects.begin(),
    objects.end(),
    objects_.begin(),
    objects_.end(),
    std::back_inserter(setDifferences));
  
  onReceivedSyncData_(setDifferences, false);
}

void 
SyncBasedDiscovery::onTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  cout << "Sync interest times out, re-expressing sync interest." << endl;
  
  // Do I or do I not want to updateHash here? I don't think that I do.
  Name interestName(broadcastPrefix_);
  interestName.append(currentDigest_);
  
  Interest newInterest(interestName);
  newInterest.setInterestLifetimeMilliseconds(defaultInterestLifetime_);
    
  face_.expressInterest
    (newInterest, bind(&SyncBasedDiscovery::onData, this, _1, _2), 
     bind(&SyncBasedDiscovery::onTimeout, this, _1));
}

void 
SyncBasedDiscovery::onInterest
  (const ptr_lib::shared_ptr<const Name>& prefix,
   const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
   uint64_t registerPrefixId)
{
  cout << "Interest " << interest->getName().toUri() << " received. Not getting answered by MCS" << endl;
  // memoryContentCache should be able to satisfy interests marked with "ANSWER_NO_CONTENT_STORE",
  // Will it cause potential problems?
  string syncDigest = interest->getName().get
    (broadcastPrefix_.size()).toEscapedString();
    
  if (syncDigest != currentDigest_) {
    // the syncDigest differs from the local knowledge, reply with local knowledge
    // We don't have digestLog implemented in this, so incremental replying is not considered now
    
    // It could potentially cause one name corresponds with different data in different locations,
    // Same as publishing two conferences simultaneously: such errors should be correctible
    // later steps
    Data data(interest->getName());
    std::string content = objectsToString();
    data.setContent((const uint8_t *)&content[0], content.size());
    data.getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);
    data.getMetaInfo().setFreshnessPeriod(defaultDataFreshnessPeriod_);
    
    keyChain_.sign(data, certificateName_);
    Blob encodedData = data.wireEncode();

    transport.send(*encodedData);
  }
  else if (syncDigest != newComerDigest_) {
    // Store this steady-state (outstanding) interest in application PIT, unless neither the sender
    // nor receiver knows anything
    pendingInterestTable_.push_back(ptr_lib::shared_ptr<PendingInterest>
      (new PendingInterest(interest, transport)));
  }
}

void 
SyncBasedDiscovery::onRegisterFailed
  (const ptr_lib::shared_ptr<const Name>& prefix)
{
  cout << "Prefix registration for name " << prefix->toUri() << " failed." << endl;
}

void
SyncBasedDiscovery::updateHash()
{
// TODO: hash implementation
/*
  SHA256_CTX sha256;

  SHA256_Init(&sha256);
  uint8_t number[4];
  int32ToLittleEndian(seqno_session_, number);
  SHA256_Update(&sha256, number, sizeof(number));
  int32ToLittleEndian(seqno_seq_, number);
  SHA256_Update(&sha256, number, sizeof(number));
  uint8_t digest_seq[SHA256_DIGEST_LENGTH];
  SHA256_Final(digest_seq, &sha256);

  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &dataPrefix_[0], dataPrefix_.size());
  uint8_t digest_name[SHA256_DIGEST_LENGTH];
  SHA256_Final(digest_name, &sha256);

  SHA256_Init(&sha256);
  SHA256_Update(&sha256, digest_name, sizeof(digest_name));
  SHA256_Update(&sha256, digest_seq, sizeof(digest_seq));
  uint8_t digest_node[SHA256_DIGEST_LENGTH];
  SHA256_Final(digest_node, &sha256);
  digest_ = toHex(digest_node, sizeof(digest_node));
*/
}

void
SyncBasedDiscovery::publishObject(std::string name)
{
  // addObject sorts the objects array
  if (addObject(name)) {
    Name dataName = Name(broadcastPrefix_).append(currentDigest_);
    Data data(dataName);
    
    std::string content = objectsToString();
    data.setContent((const uint8_t *)&content[0], content.size());
    data.getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);
    data.getMetaInfo().setFreshnessPeriod(defaultDataFreshnessPeriod_);
    
    keyChain_.sign(data, certificateName_);
    contentCacheAdd(data);
    
    updateHash();
    
    Name interestName = Name(broadcastPrefix_).append(currentDigest_);
    Interest interest(interestName);
    interest.setInterestLifetimeMilliseconds(defaultInterestLifetime_);
    
    face_.expressInterest
      (interest, bind(&SyncBasedDiscovery::onData, this, _1, _2), 
       bind(&SyncBasedDiscovery::onTimeout, this, _1));
  }
  else {
    cout << "Object already exists." << endl;
  }
}

void
SyncBasedDiscovery::contentCacheAdd(const Data& data)
{
  contentCache_.add(data);

  // Remove timed-out interests and check if the data packet matches any pending
  // interest.
  // Go backwards through the list so we can erase entries.
  MillisecondsSince1970 nowMilliseconds = ndn_getNowMilliseconds();
  for (int i = (int)pendingInterestTable_.size() - 1; i >= 0; --i) {
    if (pendingInterestTable_[i]->isTimedOut(nowMilliseconds)) {
      pendingInterestTable_.erase(pendingInterestTable_.begin() + i);
      continue;
    }

    if (pendingInterestTable_[i]->getInterest()->matchesName(data.getName())) {
      try {
        // Send to the same transport from the original call to onInterest.
        // wireEncode returns the cached encoding if available.
        pendingInterestTable_[i]->getTransport().send
          (*data.wireEncode());
      }
      catch (std::exception& e) {
      }

      // The pending interest is satisfied, so remove it.
      pendingInterestTable_.erase(pendingInterestTable_.begin() + i);
    }
  }
}

SyncBasedDiscovery::PendingInterest::PendingInterest
  (const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport)
  : interest_(interest), transport_(transport)
{
  // Set up timeoutTime_.
  if (interest_->getInterestLifetimeMilliseconds() >= 0.0)
    timeoutTimeMilliseconds_ = ndn_getNowMilliseconds() +
      interest_->getInterestLifetimeMilliseconds();
  else
    // No timeout.
    timeoutTimeMilliseconds_ = -1.0;
}