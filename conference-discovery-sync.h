// Authored by Zhehao on Aug 19, 2014
// This sync based discovery works similarly as ChronoSync, 
// however, it does not have a built-in sequence number or digest tree
// And the user does not have to be part of a digest tree to synchronize the root digest with others

#ifndef __ndnrtc__addon__conference__discovery__
#define __ndnrtc__addon__conference__discovery__

#include <ndn-cpp/ndn-cpp-config.h>

#include <ndn-cpp/util/memory-content-cache.hpp>

#include <ndn-cpp/security/identity/memory-identity-storage.hpp>
#include <ndn-cpp/security/identity/memory-private-key-storage.hpp>
#include <ndn-cpp/security/policy/no-verify-policy-manager.hpp>
#include <ndn-cpp/transport/tcp-transport.hpp>

#include <boost/algorithm/string.hpp>
#include <sys/time.h>
#include <iostream>

#include "sync-based-discovery.h"
#include "external-observer.h"
#include "conference-info-factory.h"

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;
#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
// In the std library, the placeholders are in a different namespace than boost.
using namespace func_lib::placeholders;
#endif

namespace conference_discovery
{
  class ConferenceDiscovery
  {
  public:
    /**
     * Constructor
     * @param broadcastPrefix broadcastPrefix The name prefix for broadcast. Hashes of discovered 
     * and published objects will be appended directly after broadcast prefix
     * @param observer The observer class for receiving and displaying conference discovery messages.
     * @param face The face for broadcast sync and multicast fetch interest.
     * @param keyChain The keychain to sign things with.
     * @param certificateName The certificate name for locating the certificate.
     */
	ConferenceDiscovery
	  (string broadcastPrefix, ptr_lib::shared_ptr<ConferenceDiscoveryObserver> observer, 
	   ptr_lib::shared_ptr<ConferenceInfoFactory> factory, Face& face, KeyChain& keyChain, 
	   Name certificateName)
	:  isHostingConference_(false), defaultDataFreshnessPeriod_(2000),
	   defaultInterestLifetime_(2000), defaultInterval_(2000),
	   observer_(observer), factory_(factory), faceProcessor_(face), keyChain_(keyChain), 
	   certificateName_(certificateName)
	{
	  syncBasedDiscovery_.reset(new SyncBasedDiscovery
		(broadcastPrefix, bind(&ConferenceDiscovery::onReceivedSyncData, this, _1), 
		 faceProcessor_, keyChain_, certificateName_));
	};
  
    /**
	 * Publish conference publishes conference to be discovered by the broadcastPrefix in 
	 * the constructor.
	 * It registers prefix for the intended conference name, 
	 * if local peer's not publishing before
	 * @param conferenceName string name of the conference.
	 * @param localPrefix name prefix of the conference.
	 * @param conferenceInfo the info of this conference.
	 */
	void 
	publishConference(std::string conferenceName, Name localPrefix, ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo);
  
    /**
	 * Stop publishing the conference of this instance. 
	 * Also removes the registered prefix by its ID.
	 * Note that interest with matching name could still arrive, but will not trigger
	 * onInterest.
	 */
	void
	stopPublishingConference();
	
	/**
	 * getConferences returns the copy of list of conferences
	 */
	std::map<std::string, ptr_lib::shared_ptr<ConferenceInfo>>
	getConference() { return conferenceList_; };
    
    /**
     * getConference gets the conference info from list of conferences discovered (hosted by others)
     */
    ptr_lib::shared_ptr<ConferenceInfo>
    getConference(std::string conferenceName) {
      std::map<string, ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = conferenceList_.find
        (conferenceName);
	  if (item != conferenceList_.end()) {
	    return item->second;
	  }
	  else {
	    // TODO: nullptr vs NULL
	    return NULL;
	  }
    };
    
    /**
     * getSelfConference gets the conference hosted by self;
     */
    ptr_lib::shared_ptr<ConferenceInfo>
    getSelfConference() {
      if (conferenceInfo_ != NULL) {
        return conferenceInfo_;
      }
      else {
        return NULL;
      }
    };
    
	~ConferenceDiscovery() {
  
	};
	
	
  private:
	/**
	 * onReceivedSyncData is passed into syncBasedDiscovery, and called whenever 
	 * syncData is received in syncBasedDiscovery.
	 */
	void 
	onReceivedSyncData(const std::vector<std::string>& syncData);

	/**
	 * When receiving interest about the conference hosted locally, 
	 * respond with a string that tells the interest issuer that this conference is ongoing
	 */
	void
    onInterest
	 (const ptr_lib::shared_ptr<const Name>& prefix,
	  const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
	  uint64_t registerPrefixId);
    
	/**
	 * Handles the ondata event for conference querying interest
	 * For now, whenever data is received means the conference in question is ongoing.
	 * The content should be conference description for later uses.
	 */
	void 
	onData
	  (const ptr_lib::shared_ptr<const Interest>& interest,
	   const ptr_lib::shared_ptr<Data>& data);
  
	/**
	 * expressHeartbeatInterest expresses the interest for certain conference again,
	 * to learn if the conference is still going on. This is done like heartbeat with a 2 seconds
	 * default interval. Should switch to more efficient mechanism.
	 */
	void
	expressHeartbeatInterest
	  (const ptr_lib::shared_ptr<const Interest>& interest,
	   const ptr_lib::shared_ptr<const Interest>& conferenceInterest);
  
	/**
	 * This works as expressHeartbeatInterest's onData callback.
	 * Should switch to more efficient mechanism.
	 */
	void
	onHeartbeatData
	  (const ptr_lib::shared_ptr<const Interest>& interest,
	   const ptr_lib::shared_ptr<Data>& data);
  
	void 
	dummyOnData
	  (const ptr_lib::shared_ptr<const Interest>& interest,
	   const ptr_lib::shared_ptr<Data>& data)
	{
	  cout << "Dummy onData called by ConferenceDiscovery." << endl;
	};
  
	/**
	 * Handles the timeout event for unicast conference querying interest:
	 * For now, receiving one timeout means the conference being queried is over.
	 * This strategy is immature and should be replaced.
	 */
	void
	onTimeout
	  (const ptr_lib::shared_ptr<const Interest>& interest);
  
	void
	onRegisterFailed
	  (const ptr_lib::shared_ptr<const Name>& prefix)
	{
	  cout << "Prefix " << prefix->toUri() << " registration failed." << endl;
	};
	
	std::string conferencesToString();
	
	void 
	notifyObserver(MessageTypes type, const char *msg, double timestamp);
    
    
    Face& faceProcessor_;
	KeyChain& keyChain_;
	
	Name certificateName_;
    
	bool isHostingConference_;
	Name conferenceName_;
	uint64_t registeredPrefixId_;
  
	const Milliseconds defaultDataFreshnessPeriod_;
	const Milliseconds defaultInterestLifetime_;
	const Milliseconds defaultInterval_;
  
	//vector<std::string> conferenceList_;
	
	std::map<std::string, ptr_lib::shared_ptr<ConferenceInfo>> conferenceList_;
	ptr_lib::shared_ptr<SyncBasedDiscovery> syncBasedDiscovery_;
	ptr_lib::shared_ptr<ConferenceDiscoveryObserver> observer_;
	
	ptr_lib::shared_ptr<ConferenceInfoFactory> factory_;
	// conferenceInfo_ is the info of the class currently being hosted
	ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo_;
  };
}

#endif