#include "conference-discovery-sync.h"
#include <sys/time.h>
#include <openssl/rand.h>
#include <iostream>

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;
using namespace conference_discovery;

#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
using namespace func_lib::placeholders;
#endif

void 
ConferenceDiscovery::publishConference
  (std::string conferenceName, Name localPrefix, ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo) 
{
  if (!isHostingConference_) {
	conferenceName_ = Name(localPrefix).append(conferenceName);
	registeredPrefixId_ = faceProcessor_.registerPrefix
	  (conferenceName_, 
	   bind(&ConferenceDiscovery::onInterest, this, _1, _2, _3, _4), 
	   bind(&ConferenceDiscovery::onRegisterFailed, this, _1));
	
	syncBasedDiscovery_->publishObject(conferenceName_.toUri());
	isHostingConference_ = true;
	conferenceBeingRemoved_ = "";
	
	// this destroys the parent class object.
	conferenceInfo_ = conferenceInfo;
	
	notifyObserver(MessageTypes::START, conferenceName.c_str(), 0);
  }
  else {
	cout << "Already hosting a conference." << endl;
  }
}

void
ConferenceDiscovery::removeRegisteredPrefix
  (const ptr_lib::shared_ptr<const Interest>& interest)
{
  faceProcessor_.removeRegisteredPrefix(registeredPrefixId_);
  conferenceBeingRemoved_ = "";
}

void
ConferenceDiscovery::stopPublishingConference()
{
  if (isHostingConference_) {
    // TODO: Could it go wrong if the peer stops the current conference, and
    // start one again within the interval? This judgment here shouldn't matter.
    if (conferenceBeingRemoved_ == "") {
	  isHostingConference_ = false;
	  conferenceBeingRemoved_ = conferenceName_.toUri();
	
	  Interest timeout("/timeout");
	  timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

	  faceProcessor_.expressInterest
		(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		 bind(&ConferenceDiscovery::removeRegisteredPrefix, this, _1));
	
	  syncBasedDiscovery_->stopPublishingObject(conferenceName_.toUri());
	  notifyObserver(MessageTypes::STOP, conferenceName_.toUri().c_str(), 0);
	}
	else {
	  cout << "Last conference " << conferenceBeingRemoved_ << " is still being removed." << endl;
	}
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
  
  if (interest->getName().toUri() != conferenceBeingRemoved_) {
	data.setContent(factory_->serialize(conferenceInfo_));
  } else {
    string content = "over";
    data.setContent((const uint8_t *)&content[0], content.size());
  }
  
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
  
  std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = conferenceList_.find
    (interest->getName().toUri());
  
  if (item == conferenceList_.end()) {
    if (conferenceName != "") {
      string content = "";
      for (size_t i = 0; i < data->getContent().size(); ++i) {
        content += (*data->getContent())[i];
      }
      
      if (content != "over") {
		conferenceList_.insert
		  (std::pair<string, ptr_lib::shared_ptr<ConferenceInfo>>
			(interest->getName().toUri(), factory_->deserialize(data->getContent())));
	
		// std::map should be sorted by default
		//std::sort(conferenceList_.begin(), conferenceList_.end());

		// Probably need lock for adding/removing objects in SyncBasedDiscovery class.
		// Here we update hash as well as adding object; The next interest will carry the new digest

		// Expect this to be equal with 0 several times. 
		// Because new digest does not get updated immediately
		if (syncBasedDiscovery_->addObject(interest->getName().toUri(), true) == 0) {
		  cout << "Did not add to the conferenceList_ in syncBasedDiscovery_" << endl;
		}

		notifyObserver(MessageTypes::ADD, interest->getName().toUri().c_str(), 0);

		Interest timeout("/timeout");
		timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

		// express heartbeat interest after 2 seconds of sleep
		faceProcessor_.expressInterest
		  (timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		   bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
	  }
	  else {
        cout << "received over" << endl;
	    // TODO: The chance of this happening at the exactly the same time as 
	    // a row of timeouts happen is small, but that case should still be considered.
	    
	    // For now, this part can only happen when the data stored in content cache became stale
	    
		// Same code as in timeout, should generalize as removeConference.
		if (syncBasedDiscovery_->removeObject(item->first, true) == 0) {
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
	else {
	  cout << "Conference name empty." << endl;
	}
  }
  else {
    item->second->resetTimeout();
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
  
  std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = conferenceList_.find
    (interest->getName().toUri());
  if (item != conferenceList_.end()) {
	if (item->second->incrementTimeout()) {
	  // Probably need lock for adding/removing objects in SyncBasedDiscovery class.
	  if (syncBasedDiscovery_->removeObject(item->first, true) == 0) {
		cout << "Did not remove from the conferenceList_ in syncBasedDiscovery_" << endl;
	  }
  
	  // erase the item after it's removed in removeObject, or removeObject would remove the
	  // wrong element: iterator is actually representing a position index, and the two vectors
	  // should be exactly the same: (does it make sense for them to be shared, 
	  // and mutex-locked correspondingly?)
	  conferenceList_.erase(item);
	
	  notifyObserver(MessageTypes::STOP, conferenceName.c_str(), 0);
	}
	else {
	  Interest timeout("/timeout");
	  timeout.setInterestLifetimeMilliseconds(defaultTimeoutReexpressInterval_);
      faceProcessor_.expressInterest
		(timeout, bind(&ConferenceDiscovery::dummyOnData, this, _1, _2),
		 bind(&ConferenceDiscovery::expressHeartbeatInterest, this, _1, interest));
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
	 bind(&ConferenceDiscovery::onHeartbeatData, this, _1, _2), 
	 bind(&ConferenceDiscovery::onTimeout, this, _1));
}

void
ConferenceDiscovery::onHeartbeatData
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const ptr_lib::shared_ptr<Data>& data)
{
  Interest timeout("/timeout");
  timeout.setInterestLifetimeMilliseconds(defaultHeartbeatInterval_);

	// express heartbeat interest after 2 seconds of sleep
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
	  default:						state = "Unknown"; break;
	}
	cout << state << " " << timestamp << "\t" << msg << endl;
  }
}

std::string
ConferenceDiscovery::conferencesToString()
{
  std::string result;
  for(std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator it = conferenceList_.begin(); it != conferenceList_.end(); ++it) {
	result += it->first;
	result += "\n";
  }
  if (isHostingConference_) {
	result += (" * " + conferenceName_.toUri() + "\n");
  }
  return result;
}