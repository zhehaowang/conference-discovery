// An exclusion based add-on for conference discovery for ndnrtc
// Not used for now; and not fully tested

#include "test-conference-discovery-exfil.h"

using namespace ndnrtc::chrono_chat;

using namespace boost;

ConferenceDiscovery::PendingInterest::PendingInterest
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

void 
ConferenceDiscovery::onInterest
  (const ptr_lib::shared_ptr<const Name>& prefix,
   const ptr_lib::shared_ptr<const Interest>& inst, Transport& transport,
   uint64_t registeredPrefixId)
{
  // The onDataNotFound callback for MemoryContentCache register prefix does not seem to work as expected.
  cout << "Got interest " << inst->getName().toUri() << " that can't be answered by content cache." << endl;
  // whenever data's not found, we push this interest to pendingInterestTable_.
  // Double check: do we need to know the pending interest's existence in app-PIT?
  pendingInterestTable_.push_back(ptr_lib::shared_ptr<PendingInterest>
    (new PendingInterest(inst, transport)));
}

void 
ConferenceDiscovery::onData
  (const ptr_lib::shared_ptr<const Interest>& interest, 
   const ptr_lib::shared_ptr<Data>& data)
{
  std::string content;
  std::vector<std::string> strs;
  
  cout << "Got data: " << endl;
  // Make sure it's the right toString
  for (size_t i = 0; i < data->getContent().size(); ++i) {
    cout << (*data->getContent())[i];
    content += (*data->getContent())[i];
  }
  
  boost::split(strs, content, boost::is_any_of("-"));
  
  ConferenceDescription conferenceDescription(strs[1]);
  addConference(strs[0], conferenceDescription);
  
  Exclude exclude = generateExclude();
  Interest newInterest(discoveryNamePrefix_);
  newInterest.setExclude(exclude);
  newInterest.setInterestLifetimeMilliseconds(2000);
  
  face_.expressInterest
    (newInterest, bind(&ConferenceDiscovery::onData, this, _1, _2), 
     bind(&ConferenceDiscovery::onTimeout, this, _1));
}

void 
ConferenceDiscovery::onTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  cout << "Conference discovery interest timed out..." << endl;
  
  Exclude exclude = generateExclude();
  Interest newInterest(discoveryNamePrefix_);
  
  newInterest.setExclude(exclude);
  newInterest.setInterestLifetimeMilliseconds(2000);
  
  face_.expressInterest
    (newInterest, bind(&ConferenceDiscovery::onData, this, _1, _2), 
     bind(&ConferenceDiscovery::onTimeout, this, _1));
}  

void
ConferenceDiscovery::onRegisterFailed
  (const ptr_lib::shared_ptr<const Name>& prefix)
{
  cout << "Prefix registration failed..." << endl;
}

void
ConferenceDiscovery::contentCacheAdd(const Data& data)
{
  contentCache_.add(data);

  // Remove timed-out interests and check if the data packet matches any pending
  // interest.
  // Go backwards through the list so we can erase entries.
  
  // Interesting, any time when something's added to the contentCache, we check if 
  // application-level PIT has something to be removed.
  
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

int
stopPublishingConference(const std::string conferenceName)
{
  
}

int
ConferenceDiscovery::publishConference(const std::string conferenceName, ConferenceDescription conferenceDesc)
{
  // For now, we don't check for exceptions caused by conference having the same name.
  conferenceList_.insert(std::pair<std::string, ConferenceDescription>(conferenceName, conferenceDesc));
  
  //excludeUpdateLock_.Enter();
  ExcludedConference newConference(conferenceName);
  excludedConferenceTable_.push_back(newConference);
  //excludeUpdateLock_.Leave();
  
  cout << "Conference " << conferenceName << " published." << endl;
  
  Data data(discoveryNamePrefix_);
  data.getName().append(conferenceName);
  std::string content = conferenceName + "-" + conferenceDesc.toString();
  
  data.setContent((const uint8_t *)&content[0], content.size());
  keyChain_.sign(data, certificateName_);
  // Haven't set the freshness period of the conference data yet.
  
  contentCacheAdd(data);
  
  cout << "Conference " << conferenceName << " added to contentcache." << endl;
}

int 
ConferenceDiscovery::addConference(std::string conferenceName, ConferenceDescription conferenceDesc)
{
  if (conferenceList_.find(conferenceName) == conferenceList_.end())
  {
    conferenceList_.insert(std::pair<std::string, ConferenceDescription>(conferenceName, conferenceDesc));
  }
  else
  {
    return 0;
  }
  //excludeUpdateLock_.Enter();
  if (!findExcludedConferenceByName(conferenceName)) {
    ExcludedConference newConference(conferenceName);
    excludedConferenceTable_.push_back(newConference);
  }
  //excludeUpdateLock_.Leave();
  
  return 1;
}

int 
ConferenceDiscovery::removeConference(std::string conferenceName)
{
  if (conferenceList_.find(conferenceName) != conferenceList_.end())
  {
    // erasing by key.
    conferenceList_.erase(conferenceName);
  }
}

Exclude
ConferenceDiscovery::generateExclude()
{
  Exclude exclude;
  int length = excludedConferenceTable_.size();
  int i = 0;
  
  //excludeUpdateLock_.Enter();
  for (i = 0; i < length; i++) {
    exclude.appendComponent
      ((const uint8_t *)&excludedConferenceTable_[i].getConferenceName()[0],
       excludedConferenceTable_[i].getConferenceName().size());
  }
  //excludeUpdateLock_.Leave();
  
  return exclude;
}

void *
ConferenceDiscovery::excludeUpdate()
{
  // Hard coded update interval of 3 seconds
  
  int updateInterval = 3;
  while (isExcludeUpdating_) {
    int length = excludedConferenceTable_.size();
    int i = 0;
    
    //excludeUpdateLock_.Enter();
    for (i = 0; i < length; i++) {
      if (excludedConferenceTable_[i].getDuration() - updateInterval > 0) {
        excludedConferenceTable_[i].setDuration
          (excludedConferenceTable_[i].getDuration() - updateInterval);
      }
      else {
        // TODO: double check: does erase() move indices of later elements, if so, why begin()?
        excludedConferenceTable_.erase(excludedConferenceTable_.begin() + i);
      }
    }
    //excludeUpdateLock_.Leave();
    
    cout << "Exclusion updated." << endl;
    
    // usleep in mew-seconds
    usleep(updateInterval * 1000000);
  }
  return NULL;
}

// This callHandle and static_cast are interesting techniques

void* callHandle(void *data) {
  ConferenceDiscovery *h = static_cast<ConferenceDiscovery*>(data);
  h->excludeUpdate();
}

int
ConferenceDiscovery::startExcludeUpdate()
{
  if (!isExcludeUpdating_) {
    isExcludeUpdating_ = true;
    
    // pthread does not accept member functions as third parameter...reasons and analysis
    pthread_create(&excludeUpdateThread_, NULL, &callHandle, static_cast<void*>(this));
    return 1;
  }
  else {
    return 0;
  }
}

int
ConferenceDiscovery::stopExcludeUpdate()
{
  if (isExcludeUpdating_) {
    isExcludeUpdating_ = false;
    // The thread will exit when setting isExcludeUpdating_ to false, and will exit.
    //pthread_cancel(excludeUpdateThread_);
    return 1;
  }
  else {
    return 0;
  }
}