/**
 * Copyright (C) 2014 Regents of the University of California.
 * @author: Zhehao Wang <wangzhehao410305@gmail.com>
 * An exclusion-based add-on to ChronoChat and ndnrtc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version, with the additional exemption that
 * compiling, linking, and/or using OpenSSL is allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU General Public License is in the file COPYING.
 */

// Probably should use protobuf for conference description message and so on.
// We are going to discuss the protocol specification later anyways, so I
// won't use protobuf for now.

#ifndef __test_conference_discovery_exfil__
#define __test_conference_discovery_exfil__

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

// This implementation could easily be made independent of boost, using boost for simplicity for now
#include <boost/algorithm/string.hpp>
// Boost mutex is another boost reference, which exists because std::mutex does not work with clang without further compiler flags
//#include <mutex>
// For now, switched to using webrtc::mutex; merely including boost::mutex would cause demoapp to crash
// Investigating reasons

#include <sys/time.h>

#endif

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;

namespace ndnrtc
{
  namespace chrono_chat
  {
    
    static MillisecondsSince1970 
    ndn_getNowMilliseconds()
    {
      struct timeval t;
      // Note: configure.ac requires gettimeofday.
      gettimeofday(&t, 0);
      return t.tv_sec * 1000.0 + t.tv_usec / 1000.0;
    }
    
    // Left here for later modifications: we'll use the methods in ndnrtc-namespace later at some point
    //const Name discoveryNamePrefix("/ndn/broadcast/ndnrtc/conferencelist/");
    
    // This ConferenceDescription class could be regulated by protobuf later...
    class ConferenceDescription
    {
    public:
      ConferenceDescription
        (std::string info, std::vector<std::string> participants):
         info_(info), participants_(participants)
      {
    
      };
      
      ~ConferenceDescription()
      {
      
      };
      
      // these two following methods, along with the serialization of the 
      // map<string, desc>, should be replaced by protobuf serialization
      std::string toString()
      {
        std::string result = info_ + "\n";
        int length = participants_.size();
        for (int i = 0; i < length; i++) {
          result += participants_[i];
          result += "\t";
        }
        result += "\n";
        return result;
      };
      
      ConferenceDescription(std::string inputString)
      {
        std::vector<std::string> strs;
        boost::split(strs, inputString, boost::is_any_of("\n"));
        // We are not expecting exceptions for now
        info_ = strs[0];
        
        std::vector<std::string> pars;
        boost::split(pars, strs[1], boost::is_any_of("\t"));
        int i = 0;
        for (i = 0; i < pars.size(); i++) {
          if (pars[i] != "")
            participants_.push_back(pars[i]);
        }
      };
      
      std::vector<std::string> getParticipants() { return participants_; };
        
      std::string getInfo() { return info_; };
      
    private:
      std::string info_;
      std::vector<std::string> participants_;
      // Duration is not used for now, and it probably won't be an int later.
      int duration_;
    };
    
    class ConferenceDiscovery
    {
    public:
      ConferenceDiscovery
        (Name discoveryNamePrefix, Face& face, KeyChain& keyChain, Name certificateName)
        : discoveryNamePrefix_(discoveryNamePrefix), face_(face), 
          keyChain_(keyChain), certificateName_(certificateName), contentCache_(&face), 
          isExcludeUpdating_(false)
          //, excludeUpdateLock_(*webrtc::CriticalSectionWrapper::CreateCriticalSection())
      {
        face_.registerPrefix
          (discoveryNamePrefix_, 
           bind(&ConferenceDiscovery::onInterest, this, _1, _2, _3, _4),
           onRegisterFailed);
        
        // Use our onInterest as the onDataNotFound for contentCache prefix registration
        contentCache_.registerPrefix
          (discoveryNamePrefix_, onRegisterFailed, 
           bind(&ConferenceDiscovery::onInterest, this, _1, _2, _3, _4));
        
        cout << "Prefix registered: " << discoveryNamePrefix_.toUri() << endl;
        
        Interest conferenceDiscoveryInterest(discoveryNamePrefix);
        conferenceDiscoveryInterest.setInterestLifetimeMilliseconds(2000);
        face_.expressInterest
          (conferenceDiscoveryInterest, bind(&ConferenceDiscovery::onData, this, _1, _2), 
           bind(&ConferenceDiscovery::onTimeout, this, _1));
           
        cout << "Interest expressed: " << conferenceDiscoveryInterest.getName().toUri() << endl;
      };
      
      ~ConferenceDiscovery()
      {
      
      };
      
      int publishConference
        (const std::string conferenceName, ConferenceDescription conferenceDesc);
      
      void contentCacheAdd(const Data& data);
      
      void onInterest
        (const ptr_lib::shared_ptr<const Name>& prefix,
		 const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
		 uint64_t registeredPrefixId);
	  
	  void onData
	    (const ptr_lib::shared_ptr<const Interest>& interest, 
	     const ptr_lib::shared_ptr<Data>& data);
	    
	  void onTimeout
	    (const ptr_lib::shared_ptr<const Interest>& interest);
	  
	  static void onRegisterFailed
	    (const ptr_lib::shared_ptr<const Name>& prefix);
	
	  /**
	   * Return the list of conferences.
	   */
	  std::map<std::string, ConferenceDescription >
		getConferenceList() { return conferenceList_; }
		
	  /**
	   * Add a newly discovered conference.
	   */
	  int 
	  addConference(std::string conferenceName, ConferenceDescription conferenceDesc);
		
	  /**
	   * Delete an existing conference by name.
	   */
	  int 
	  removeConference(std::string conferenceName);
	    
	  /**
	   * Stop publishing a conference;
	   * This removes the data from content cache about a conference that's published by local peer.
	   * Would it be sufficient if I just remove from content cache, and let other copies time out?
	   * Which means, the copies in cache times out, but the one in local memory content cache does not.
	   * No notification mechanism does not sound like the right way to go, neither does this exclusion-based discovery.
	   */
	  int
	  stopPublishingConference(const std::string conferenceName);  
	  
	  Exclude
	  generateExclude();
	  
	  void *
	  excludeUpdate();
	  
	  int
	  startExcludeUpdate();
	  
	  int
	  stopExcludeUpdate();
	  
    private:
      // A PendingInterest class similar with that of chrono-sync2013 exists. 
	  /**
	   * A PendingInterest holds an interest which onInterest received but could
	   * not satisfy. When we add a new data packet to the contentCache_, we will
	   * also check if it satisfies a pending interest.
	   */
	  class PendingInterest {
	  public:
		/**
		 * Create a new PendingInterest and set the timeoutTime_ based on the current time and the interest lifetime.
		 * @param interest A shared_ptr for the interest.
		 * @param transport The transport from the onInterest callback. If the
		 * interest is satisfied later by a new data packet, we will send the data
		 * packet to the transport.
		 */
		PendingInterest
		  (const ptr_lib::shared_ptr<const Interest>& interest,
		   Transport& transport);

		/**
		 * Return the interest given to the constructor.
		 */
		const ptr_lib::shared_ptr<const Interest>&
		getInterest() { return interest_; }

		/**
		 * Return the transport given to the constructor.
		 */
		Transport&
		getTransport() { return transport_; }
		
		/**
		 * Check if this interest is timed out.
		 * @param nowMilliseconds The current time in milliseconds from ndn_getNowMilliseconds.
		 * @return true if this interest timed out, otherwise false.
		 */
		bool
		isTimedOut(MillisecondsSince1970 nowMilliseconds)
		{
		  return timeoutTimeMilliseconds_ >= 0.0 && nowMilliseconds >= timeoutTimeMilliseconds_;
		}

	  private:
		ptr_lib::shared_ptr<const Interest> interest_;
		Transport& transport_;
		MillisecondsSince1970 timeoutTimeMilliseconds_; /**< The time when the
		  * interest times out in milliseconds according to ndn_getNowMilliseconds,
		  * or -1 for no timeout. */
	  };
	  
	  /**
	   * An ExcludedConference class defines an exclusion and its remaining time.
	   * It does not have a 'very direct' relationship with the list of ongoing conferences.
	   */
	  class ExcludedConference
	  {
	  public:
	    // The lasting re-check duration for a default conference is 100s
	    ExcludedConference(std::string conferenceName)
	     : duration_(100), conferenceName_(conferenceName)
	    {
	    
	    };
	    
	    ~ExcludedConference()
	    {
	    
	    };
	    
	    int getDuration() { return duration_; };
	    
	    void setDuration(int duration) { duration_ = duration; }; 
	    
	    std::string getConferenceName() { return conferenceName_; };
	    
	  private:
	    // duration records the time 
	    int duration_;
	    std::string conferenceName_;
	  };
	  
	  bool findExcludedConferenceByName(std::string name)
	  {
	    for (int i = 0; i < excludedConferenceTable_.size(); i++) {
          if (excludedConferenceTable_[i].getConferenceName() == name) {
            return true;
          }
        }
        return false;
	  };
	  
      Name discoveryNamePrefix_;
    
      Face &face_;
      KeyChain &keyChain_;
      Name certificateName_;
      
      MemoryContentCache contentCache_;
      std::vector<ptr_lib::shared_ptr<PendingInterest> > pendingInterestTable_;
      // using boost::shared_ptr here vs not: do I want this class to store the actual stuff
      // if so, then this way sounds better
      std::vector<ExcludedConference> excludedConferenceTable_;
      std::map<std::string, ConferenceDescription> conferenceList_;
      
      // Exclusion filter update does not necessarily need to use thread,
      // sounds like a timer would be enough
      pthread_t excludeUpdateThread_;
      
      // For the test, the lock is not used. 
      //webrtc::CriticalSectionWrapper &excludeUpdateLock_;
      bool isExcludeUpdating_;
    };
    
  }
}