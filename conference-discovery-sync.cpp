#include "conference-discovery-sync.h"
#include <sys/time.h>
#include <openssl/ssl.h>
#include <iostream>

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;
using namespace conference_discovery;

#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
using namespace func_lib::placeholders;
#endif

void 
ConferenceDiscovery::publishConference(std::string conferenceName, Name localPrefix) 
{
  if (!isHostingConference_) {
	conferenceName_ = Name(localPrefix).append(conferenceName);
	registeredPrefixId_ = faceProcessor_.registerPrefix
	  (conferenceName_, 
	   bind(&ConferenceDiscovery::onInterest, this, _1, _2, _3, _4), 
	   bind(&ConferenceDiscovery::onRegisterFailed, this, _1));
	// TODO: do I use to escaped string or toUri here?
	// should test in onReceivedSyncData, and see if the interest is constructed correctly
	syncBasedDiscovery_->publishObject(conferenceName_.toUri());
	isHostingConference_ = true;
	
	notifyObserver(MessageTypes::START, conferenceName.c_str(), 0);
  }
  else {
	cout << "Already hosting a conference." << endl;
  }
}

void
ConferenceDiscovery::stopPublishingConference()
{
  if (isHostingConference_) {
	faceProcessor_.removeRegisteredPrefix(registeredPrefixId_);
	isHostingConference_ = false;
	
	// TODO: I should add a stopPublishingConference method to this
	syncBasedDiscovery_->stopPublishingObject(conferenceName_.toUri());
	notifyObserver(MessageTypes::STOP, conferenceName_.toUri().c_str(), 0);
  }
  else {
	cout << "Not hosting any conferences." << endl;
  }
}

void 
ConferenceDiscovery::onReceivedSyncData
  (const std::vector<std::string>& syncData)
{
  ptr_lib::shared_ptr<Interest> interest;

  // For every name received from sync, express interest to ask if they are valid;
  // Could try answer_origin_kind for this, but not scalable.
  for (size_t j = 0; j < syncData.size(); ++j) {
	if (syncData[j] != conferenceName_.toUri()) {
	  interest.reset(new Interest(syncData[j]));
	  interest->setInterestLifetimeMilliseconds(defaultInterestLifetime_);
  
	  // Using bind vs not?
	  faceProcessor_.expressInterest
		(*(interest.get()), bind(&ConferenceDiscovery::onData, this, _1, _2),
		 bind(&ConferenceDiscovery::onTimeout, this, _1));
	}
  }
}

void
ConferenceDiscovery::onInterest
  (const ptr_lib::shared_ptr<const Name>& prefix,
   const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
   uint64_t registerPrefixId)
{
  Data data(interest->getName());

  // Should be replaced with conference description later
  std::string content = "ongoing";

  data.setContent((const uint8_t *)&content[0], content.size());
  data.getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);
  data.getMetaInfo().setFreshnessPeriod(defaultDataFreshnessPeriod_);

  keyChain_.sign(data, certificateName_);
  Blob encodedData = data.wireEncode();

  transport.send(*encodedData);
}

void 
ConferenceDiscovery::onData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  std::string conferenceName = interest->getName().get
	(-1).toEscapedString();

  std::vector<std::string>::iterator item = std::find
	(conferenceList_.begin(), conferenceList_.end(), interest->getName().toUri());

  if (item == conferenceList_.end() && conferenceName != "") {
	 conferenceList_.push_back(interest->getName().toUri());
	 std::sort(conferenceList_.begin(), conferenceList_.end());
   
	 // Probably need lock for adding/removing objects in SyncBasedDiscovery class.
	 // Here we update hash as well as adding object; The next interest will carry the new digest
   
	 // Expect this to be equal with 0 several times. 
	 // Because new digest does not get updated immediately
	if (syncBasedDiscovery_->addObject(interest->getName().toUri(), true) == 0) {
	  cout << "Did not add to the conferenceList_ in syncBasedDiscovery_" << endl;
	}
	
	notifyObserver(MessageTypes::ADD, conferenceName.c_str(), 0);
	
	// Express interest periodically to know if the conference is still going on.
	// Uses timeout mechanism to handle the sleep period
	Interest timeout("/timeout");
	timeout.setInterestLifetimeMilliseconds(defaultInterval_);

	// express broadcast interest after 2 seconds of sleep
	faceProcessor_.expressInterest
	  (timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
	   bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
  }
}

void
ConferenceDiscovery::onTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  // the last component should be the name of the conference itself
  std::string conferenceName = interest->getName().get
	(-1).toEscapedString();
  
  std::vector<std::string>::iterator item = std::find
	(conferenceList_.begin(), conferenceList_.end(), interest->getName().toUri());
  if (item != conferenceList_.end()) {
   
	 // Probably need lock for adding/removing objects in SyncBasedDiscovery class.
	 if (syncBasedDiscovery_->removeObject(*item, true) == 0) {
	   cout << "Did not remove from the conferenceList_ in syncBasedDiscovery_" << endl;
	 }
   
	 // erase the item after it's removed in removeObject, or removeObject would remove the
	 // wrong element: iterator is actually representing a position index, and the two vectors
	 // should be exactly the same: (does it make sense for them to be shared, 
	 // and mutex-locked correspondingly?)
	 conferenceList_.erase(item);
	 
	 notifyObserver(MessageTypes::STOP, conferenceName.c_str(), 0);
  }
}

void
ConferenceDiscovery::expressHeartbeatInterest
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<const Interest>& conferenceInterest)
{
  faceProcessor_.expressInterest
	(*(conferenceInterest.get()),
	 bind(&ConferenceDiscovery::onHeartbeatData, this, _1, _2), 
	 bind(&ConferenceDiscovery::onTimeout, this, _1));
}

void
ConferenceDiscovery::onHeartbeatData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  Interest timeout("/timeout");
  timeout.setInterestLifetimeMilliseconds(defaultInterval_);

	// express broadcast interest after 2 seconds of sleep
  faceProcessor_.expressInterest
	(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
	 bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
}

void 
ConferenceDiscovery::notifyObserver(MessageTypes type, const char *msg, double timestamp)
{
  if (observer_) {
	observer_->onStateChanged(type, msg, timestamp);
  }
  else {
	string state = "";
	switch (type) {
	  case MessageTypes::ADD: 		state = "Add"; break;
	  case MessageTypes::REMOVE:	state = "Remove"; break;
	  case MessageTypes::SET:		state = "Set"; break;
	  case MessageTypes::START:		state = "Start"; break;
	  case MessageTypes::STOP:		state = "Stop"; break;
	  default:		state = "Unknown"; break;
	}
	cout << state << " " << timestamp << "\t" << msg << endl;
  }
}

std::string
ConferenceDiscovery::conferencesToString()
{
  std::string result;
  for(std::vector<std::string>::iterator it = conferenceList_.begin(); it != conferenceList_.end(); ++it) {
	result += *it;
	result += "\n";
  }
  if (isHostingConference_) {
	result += (" * " + conferenceName_.toUri() + "\n");
  }
  return result;
}