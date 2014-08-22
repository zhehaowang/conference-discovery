// Authored by Zhehao on Aug 19, 2014
// This sync based discovery works similarly as ChronoSync, 
// however, it does not have a built-in sequence number or digest tree
// And the user does not have to be part of a digest tree to synchronize the root digest with others

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
using namespace ndn::func_lib;
using namespace ndn;

namespace conference_discovery
{
  /**
   * This class, upon instantiation, will register a broadcast prefix and 
   * will handle all the logics going on in that broadcast namespace,
   * Namely periodical broadcast of sync digest, maintenance of outstanding interest, etc
   *
   * Its idea and implementations are pretty similar with ChronoSync, but instead of 
   * modifying ChronoSync source, I decided to re-implement this.
   */
  class SyncBasedDiscovery
  {
  public:
    // For now, recovery mechanism in Chronos is still in there, but not used, 
    // because we don't have a digest log implementation to judge recovery yet.
    // cpp way of passing function pointers
    typedef func_lib::function<void
      (const std::vector<std::string>& syncData, bool isRecovery)>
        OnReceivedSyncData;
  
    // Double check: does "const int& something" make sense?
    // Does initiation part in constructor implementation do a copy, or merely a reference?
    // Question: does this initiation part call the constructor, or do merely an equal: 
    // seems to be the former, which means, it copies, unless the variable being initialized is a reference member
    
    // This class does not own the face. Their relationships are aggregation
    SyncBasedDiscovery
      (Name broadcastPrefix, const OnReceivedSyncData& onReceivedSyncData, 
       Face& face, KeyChain& keyChain, Name certificateName)
     : broadcastPrefix_(broadcastPrefix), onReceivedSyncData_(onReceivedSyncData), 
       newComerDigest_("00"), face_(face), keyChain_(keyChain), 
       certificateName_(certificateName), contentCache_(&face), 
       currentDigest_(newComerDigest_)
    {
      // Storing it in contentCache, the idea is that a set of strings maps to a digest
      contentCache_.registerPrefix
        (broadcastPrefix, bind(&SyncBasedDiscovery::onRegisterFailed, this, _1), 
         bind(&SyncBasedDiscovery::onInterest, this, _1, _2, _3, _4));
      
      Interest interest(broadcastPrefix);
      interest.getName().append(newComerDigest_);
      interest.setInterestLifetimeMilliseconds(1000);
      interest.setAnswerOriginKind(ndn_Interest_ANSWER_NO_CONTENT_STORE);
  
      face_.expressInterest
        (interest, bind(&SyncBasedDiscovery::onData, this, _1, _2),
         bind(&SyncBasedDiscovery::onTimeout, this, _1));
    };
    
    ~SyncBasedDiscovery()
    {
      
    };
    
    void onData
      (const ptr_lib::shared_ptr<const Interest>& interest,
       const ptr_lib::shared_ptr<Data>& data);
      
    void onTimeout
      (const ptr_lib::shared_ptr<const Interest>& interest);
    
    void onInterest
      (const ptr_lib::shared_ptr<const Name>& prefix,
       const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
       uint64_t registerPrefixId);
    
    void onRegisterFailed
      (const ptr_lib::shared_ptr<const Name>& prefix);
    
	const std::string newComerDigest_;
    
    /**
     * These functions should be replaced, once we replace objects with something more
     * generic
     */
    std::vector<std::string> getObjects() { return objects_; };
    
    int addObject(std::string object) {
      objects_.push_back(object);
      // Using default comparison '<' here
      std::sort(objects_.begin(), objects_.end()); 
      // Update the currentDigest_ 
      return 1;
    };
    
    int removeObject(std::string object) {
      std::vector<std::string>::iterator item = std::find(objects_.begin(), objects_.end(), object);
      if (item != objects_.end()) {
        // I should not need to sort the thing again, if it's just erase. Remains to be tested
        objects_.erase(item);
        // Update the currentDigest_
        return 1;
      }
      else {
        return 0;
      }
    };
    
  private:
    Name broadcastPrefix_;
    Name certificateName_;
    
    OnReceivedSyncData onReceivedSyncData_;
    
    Face& face_;
    MemoryContentCache contentCache_;
    
    KeyChain& keyChain_;
    
    // This serves as the rootDigest in ChronoSync
    std::string currentDigest_;
    // This serves as the list of objects to be synchronized, for now.
    // Could be replaced with a Protobuf class or a class later
    std::vector<std::string> objects_;
  };
}