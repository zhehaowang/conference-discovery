#include "conference-discovery-sync.h"
#include "sync-based-discovery.h"
#include <sys/time.h>
#include <openssl/rand.h>
#include <iostream>

#include <algorithm>

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;
using namespace conference_discovery;

#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
using namespace func_lib::placeholders;
#endif

bool 
ConferenceDiscovery::publishConference
  (std::string conferenceName, Name localPrefix, ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo) 
{
  Name prefixName = Name(localPrefix).append(conferenceName);
	
  std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = hostedConferenceList_.find(prefixName.toUri());
  if (item == hostedConferenceList_.end()) {

	uint64_t registeredPrefixId = faceProcessor_.registerPrefix
	  (prefixName, 
	   bind(&ConferenceDiscovery::onInterest, this, _1, _2, _3, _4), 
	   bind(&ConferenceDiscovery::onRegisterFailed, this, _1));
  
	syncBasedDiscovery_->publishObject(prefixName.toUri());
  
	// this destroys the parent class object.
	ptr_lib::shared_ptr<ConferenceInfo> info = conferenceInfo;
	info->setRegisteredPrefixId(registeredPrefixId);
  
	hostedConferenceList_.insert
	  (std::pair<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>(prefixName.toUri(), info));
  
	notifyObserver(MessageTypes::START, conferenceName.c_str(), 0);
	hostedConferenceNum_ ++;
	return true;
  }
  else {
    cerr << "Conference with this name already exists locally." << endl;
	return false;
  }
}

void
ConferenceDiscovery::removeRegisteredPrefix
  (const ptr_lib::shared_ptr<const Interest>& interest,
   Name conferenceName)
{ 
  std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = hostedConferenceList_.find(conferenceName.toUri());
  if (item != hostedConferenceList_.end()) {
    faceProcessor_.removeRegisteredPrefix(item->second->getRegisteredPrefixId());
    hostedConferenceList_.erase(item);
    hostedConferenceNum_ --;
  }
  else {
	cerr << "No such conference exist." << endl;
  }
}

bool
ConferenceDiscovery::stopPublishingConference
  (std::string conferenceName, ndn::Name prefix)
{
  if (hostedConferenceNum_ > 0) {
    Name conferenceBeingStopped = Name(prefix).append(conferenceName);
    
    std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = hostedConferenceList_.find(conferenceBeingStopped.toUri());
  
	if (item != hostedConferenceList_.end()) {
      item->second->setBeingRemoved(true);
      syncBasedDiscovery_->removeObject(conferenceBeingStopped.toUri(), true);
      
      Interest timeout("/localhost/timeout");
	  timeout.setInterestLifetimeMilliseconds(defaultKeepPeriod_);

	  faceProcessor_.expressInterest
		(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		 bind(&ConferenceDiscovery::removeRegisteredPrefix, this, _1, conferenceBeingStopped));
	
	  notifyObserver(MessageTypes::STOP, conferenceBeingStopped.toUri().c_str(), 0);
	  
	  return true;
    }
    else {
      cerr << "No such conference exist." << endl;
      return false;
    }
  }
  else {
	cerr << "Not hosting any conferences." << endl;
	return false;
  }
}

void 
ConferenceDiscovery::onReceivedSyncData
  (const std::vector<std::string>& syncData)
{
  for (size_t j = 0; j < syncData.size(); ++j) {
    std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator hostedItem = hostedConferenceList_.find(syncData[j]);
    std::vector<std::string>::iterator queriedItem = std::find(queriedConferenceList_.begin(), queriedConferenceList_.end(), syncData[j]);
    
    if (hostedItem == hostedConferenceList_.end() && queriedItem == queriedConferenceList_.end()) {
      queriedConferenceList_.push_back(syncData[j]);
    
	  Name name(syncData[j]);
	  Interest interest(name);
	  
	  interest.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);
      interest.setMustBeFresh(true);
      
      faceProcessor_.expressInterest
		(interest, bind(&ConferenceDiscovery::onData, this, _1, _2),
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
  std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = hostedConferenceList_.find(interest->getName().toUri());
  
  if (item != hostedConferenceList_.end()) {
  
	Data data(interest->getName());
  
	if (item->second->getBeingRemoved() == false) {
	  data.setContent(factory_->serialize(item->second));
	} else {
	  string content("over");
	  data.setContent((const uint8_t *)&content[0], content.size());
	}
    
	data.getMetaInfo().setFreshnessPeriod(defaultDataFreshnessPeriod_);

	keyChain_.sign(data, certificateName_);
	Blob encodedData = data.wireEncode();

	transport.send(*encodedData);
  }
  else {
    cerr << "Received interest about conference not hosted by this instance." << endl;
  }
}

void 
ConferenceDiscovery::onData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  std::string conferenceName = interest->getName().get
	(-1).toEscapedString();
  
  std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = discoveredConferenceList_.find
    (interest->getName().toUri());
  
  string content = "";
  for (size_t i = 0; i < data->getContent().size(); ++i) {
	content += (*data->getContent())[i];
  }
  
  // if it's not an already discovered conference
  if (item == discoveredConferenceList_.end()) {
	// if it's still going on
	if (content != "over") {
	  ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo = factory_->deserialize(data->getContent());
	  
	  if (conferenceInfo) {
	    discoveredConferenceList_.insert
		(std::pair<string, ptr_lib::shared_ptr<ConferenceInfo>>
		  (interest->getName().toUri(), conferenceInfo));
  
		// std::map should be sorted by default
		//std::sort(discoveredConferenceList_.begin(), discoveredConferenceList_.end());

		// Probably need lock for adding/removing objects in SyncBasedDiscovery class.
		// Here we update hash as well as adding object; The next interest will carry the new digest

		// Expect this to be equal with 0 several times. 
		// Because new digest does not get updated immediately
		if (syncBasedDiscovery_->addObject(interest->getName().toUri(), true) == 0) {
		  cerr << "Did not add to the discoveredConferenceList_ in syncBasedDiscovery_" << endl;
		}

		notifyObserver(MessageTypes::ADD, interest->getName().toUri().c_str(), 0);

		Interest timeout("/localhost/timeout");
		timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

		// express heartbeat interest after 2 seconds of sleep
		faceProcessor_.expressInterest
		  (timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		   bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
	  }
	  else {
	    // If received conferenceInfo is malformed, 
	    // re-express interest after a timeout.
	    Interest timeout("/localhost/timeout");
		timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

		// express heartbeat interest after 2 seconds of sleep
		faceProcessor_.expressInterest
		  (timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		   bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
	  }
	}
	// if the not already discovered conference is already over.
	else {
	  std::vector<string>::iterator queriedItem = std::find
        (queriedConferenceList_.begin(), queriedConferenceList_.end(), interest->getName().toUri());
      if (queriedItem != queriedConferenceList_.end()) {
	    queriedConferenceList_.erase(queriedItem);
	  }
	}
  }
  // if it's an already discovered conference
  else {
    if (content != "over") {
      item->second->resetTimeout();
      
      Interest timeout("/localhost/timeout");
	  timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

	  // express heartbeat interest after 2 seconds of sleep
	  faceProcessor_.expressInterest
		(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		 bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
    }
    else {
      notifyObserver(MessageTypes::REMOVE, conferenceName.c_str(), 0);
      
      if (syncBasedDiscovery_->removeObject(item->first, true) == 0) {
		cerr << "Did not remove from the discoveredConferenceList_ in syncBasedDiscovery_" << endl;
	  }
	  std::vector<string>::iterator queriedItem = std::find
        (queriedConferenceList_.begin(), queriedConferenceList_.end(), interest->getName().toUri());
      if (queriedItem != queriedConferenceList_.end()) {
	    queriedConferenceList_.erase(queriedItem);
	  }
	  discoveredConferenceList_.erase(item);
    }
  }
}

/**
 * When interest times out, increment the timeout count for that conference;
 * If timeout count reaches maximum, delete conference;
 * If not, express interest again.
 */
void
ConferenceDiscovery::onTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  // the last component should be the name of the conference itself
  std::string conferenceName = interest->getName().get
	(-1).toEscapedString();
  
  std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = discoveredConferenceList_.find
    (interest->getName().toUri());
  if (item != discoveredConferenceList_.end()) {
	if (item->second && item->second->incrementTimeout()) {
	  notifyObserver(MessageTypes::REMOVE, conferenceName.c_str(), 0);
	  
	  // Probably need lock for adding/removing objects in SyncBasedDiscovery class.
	  if (syncBasedDiscovery_->removeObject(item->first, true) == 0) {
		cerr << "Did not remove from the discoveredConferenceList_ in syncBasedDiscovery_" << endl;
	  }
  
	  // erase the item after it's removed in removeObject, or removeObject would remove the
	  // wrong element: iterator is actually representing a position index, and the two vectors
	  // should be exactly the same: (does it make sense for them to be shared, 
	  // and mutex-locked correspondingly?)
	  discoveredConferenceList_.erase(item);
	  
	  std::vector<string>::iterator queriedItem = std::find
        (queriedConferenceList_.begin(), queriedConferenceList_.end(), interest->getName().toUri());
      if (queriedItem != queriedConferenceList_.end()) {
	    queriedConferenceList_.erase(queriedItem);
	  }
	}
	else {
	  Interest timeout("/timeout");
	  timeout.setInterestLifetimeMilliseconds(defaultTimeoutReexpressInterval_);
      faceProcessor_.expressInterest
		(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		 bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
	}
  }
  else {
    std::vector<string>::iterator queriedItem = std::find
      (queriedConferenceList_.begin(), queriedConferenceList_.end(), interest->getName().toUri());
	if (queriedItem != queriedConferenceList_.end()) {
	  queriedConferenceList_.erase(queriedItem);
	} 
  }
}

void
ConferenceDiscovery::expressHeartbeatInterest
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<const Interest>& conferenceInterest)
{
  faceProcessor_.expressInterest
	(*(conferenceInterest.get()),
	 bind(&ConferenceDiscovery::onData, this, _1, _2), 
	 bind(&ConferenceDiscovery::onTimeout, this, _1));
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
	  default:						state = "Unknown"; break;
	}
	cout << state << " " << timestamp << "\t" << msg << endl;
  }
}

std::string
ConferenceDiscovery::conferencesToString()
{
  std::string result;
  for(std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator it = discoveredConferenceList_.begin(); it != discoveredConferenceList_.end(); ++it) {
	result += it->first;
	result += "\n";
  }
  for(std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator it = hostedConferenceList_.begin(); it != hostedConferenceList_.end(); ++it) {
	result += (" * " + it->first);
	result += "\n";
  }
  return result;
}