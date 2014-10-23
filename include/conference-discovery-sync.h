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

#include <sys/time.h>
#include <iostream>

#include "sync-based-discovery.h"
#include "external-observer.h"
#include "conference-info-factory.h"

// using namespace std;
// using namespace ndn;
// using namespace ndn::func_lib;
#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
// In the std library, the placeholders are in a different namespace than boost.
// using namespace func_lib::placeholders;
#endif

// TODO: Constantly getting 'add conference' message during a test.
// TODO: Different lifetimes could cause interest towards stopped self hosted conference to get reissued;
//   explicit conference over still being verified.

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
	  (std::string broadcastPrefix, ndn::ptr_lib::shared_ptr<ConferenceDiscoveryObserver> observer, 
	   ndn::ptr_lib::shared_ptr<ConferenceInfoFactory> factory, ndn::Face& face, ndn::KeyChain& keyChain, 
	   ndn::Name certificateName)
	:  defaultDataFreshnessPeriod_(1000), defaultInterestLifetime_(1000), 
	   defaultHeartbeatInterval_(1000), defaultTimeoutReexpressInterval_(300), 
	   observer_(observer), factory_(factory), faceProcessor_(face), keyChain_(keyChain), 
	   certificateName_(certificateName), hostedConferenceNum_(0)
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
	 * @return true, if conference name is not already published by this instance; false if otherwise.
	 */
	bool 
	publishConference(std::string conferenceName, ndn::Name localPrefix, ndn::ptr_lib::shared_ptr<ConferenceInfo> conferenceInfo);
  
    /**
	 * Stop publishing the conference of this instance. 
	 * Also removes the registered prefix by its ID.
	 * Note that interest with matching name could still arrive, but will not trigger
	 * onInterest.
	 * @param conferenceName string name of the conference to be stopped
	 * @param prefix name prefix of the conference to be stopped
	 * @return true, if conference with that name is published by this instance; false if otherwise.
	 */
	bool
	stopPublishingConference(std::string conferenceName, ndn::Name prefix);
	
	/**
	 * getDiscoveredConferenceList returns the copy of list of discovered conferences
	 */
	std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>
	getDiscoveredConferenceList() { return discoveredConferenceList_; };
    
    /**
	 * getHostedConferenceList returns the copy of list of discovered conferences
	 */
	std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>
	getHostedConferenceList() { return hostedConferenceList_; };
    
    /**
     * getConference gets the conference info from list of conferences discovered or hosted
     * The first conference with matching name will get returned.
     */
    ndn::ptr_lib::shared_ptr<ConferenceInfo>
    getConference(std::string conferenceName) 
    {
      std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator item = discoveredConferenceList_.find
        (conferenceName);
	  if (item != discoveredConferenceList_.end()) {
	    return item->second;
	  }
	  else {
	    std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>>::iterator hostedItem = hostedConferenceList_.find
          (conferenceName);
        if (hostedItem != hostedConferenceList_.end()) {
          return hostedItem->second;
        }
        else {
	      return ndn::ptr_lib::shared_ptr<ConferenceInfo>(NULL);
	    }
	  }
    };
    
	~ConferenceDiscovery() 
	{
  
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
	 (const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix,
	  const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, ndn::Transport& transport,
	  uint64_t registerPrefixId);
    
	/**
	 * Handles the ondata event for conference querying interest
	 * For now, whenever data is received means the conference in question is ongoing.
	 * The content should be conference description for later uses.
	 */
	void 
	onData
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
	   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
  
	/**
	 * expressHeartbeatInterest expresses the interest for certain conference again,
	 * to learn if the conference is still going on. This is done like heartbeat with a 2 seconds
	 * default interval. Should switch to more efficient mechanism.
	 */
	void
	expressHeartbeatInterest
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
	   const ndn::ptr_lib::shared_ptr<const ndn::Interest>& conferenceInterest);
    
    /**
     * Remove registered prefix happens after a few seconds after stop hosting conference;
     * So that other peers may fetch "conference over" with heartbeat interest.
     */
    void
    removeRegisteredPrefix
      (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
       ndn::Name conferenceName);
    
	/**
	 * This works as expressHeartbeatInterest's onData callback.
	 * Should switch to more efficient mechanism.
	 */
	void
	onHeartbeatData
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
	   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
  
	void 
	dummyOnData
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
	   const ndn::ptr_lib::shared_ptr<ndn::Data>& data)
	{
	  std::cout << "Dummy onData called by ConferenceDiscovery." << std::endl;
	};
  
	/**
	 * Handles the timeout event for unicast conference querying interest:
	 * For now, receiving one timeout means the conference being queried is over.
	 * This strategy is immature and should be replaced.
	 */
	void
	onTimeout
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);
  
	void
	onRegisterFailed
	  (const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix)
	{
	  std::cout << "Prefix " << prefix->toUri() << " registration failed." << std::endl;
	};
	
	std::string conferencesToString();
	
	void 
	notifyObserver(MessageTypes type, const char *msg, double timestamp);
    
    ndn::Face& faceProcessor_;
	ndn::KeyChain& keyChain_;
	
	ndn::Name certificateName_;
    
	int hostedConferenceNum_;
	
	const ndn::Milliseconds defaultDataFreshnessPeriod_;
	const ndn::Milliseconds defaultInterestLifetime_;
	const ndn::Milliseconds defaultHeartbeatInterval_;
	const ndn::Milliseconds defaultTimeoutReexpressInterval_;
  
	// List of discovered conference.
	std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>> discoveredConferenceList_;
	// List of hosted conference.
	std::map<std::string, ndn::ptr_lib::shared_ptr<ConferenceInfo>> hostedConferenceList_;
	
	ndn::ptr_lib::shared_ptr<SyncBasedDiscovery> syncBasedDiscovery_;
	ndn::ptr_lib::shared_ptr<ConferenceDiscoveryObserver> observer_;
	
	ndn::ptr_lib::shared_ptr<ConferenceInfoFactory> factory_;
  };
}

#endif