/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>, 
 * Incorporated directly into ndnrtc by Zhehao Wang. <wangzhehao410305@gmail.com>
 * Derived from ChronoChat-js by Qiuhan Ding and Wentao Shang.
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

// Only compile if ndn-cpp-config.h defines NDN_CPP_HAVE_PROTOBUF = 1.
#ifndef __ndnrtc__addon__chrono__chat__
#define __ndnrtc__addon__chrono__chat__

#include <ndn-cpp/ndn-cpp-config.h>
#if NDN_CPP_HAVE_PROTOBUF

#include <cstdlib>
#include <string>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <openssl/rand.h>

#include <ndn-cpp/sync/chrono-sync2013.hpp>
#include <ndn-cpp/security/identity/memory-identity-storage.hpp>
#include <ndn-cpp/security/identity/memory-private-key-storage.hpp>
#include <ndn-cpp/security/policy/no-verify-policy-manager.hpp>
#include <ndn-cpp/transport/tcp-transport.hpp>

#include "external-observer.h"

#if NDN_CPP_HAVE_TIME_H
#include <time.h>
#endif
#if NDN_CPP_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

namespace chrono_chat
{
  class Chat
  {
  public:
    /**
     * Constructor
     * @param broadcastPrefix The prefix for broadcast digest-tree sync namespace.
     * @param screenName The name string of this participant in the chatroom.
     * @param chatRoom The name string of chatroom.
     * @param hubPrefix The chat prefix of this participant in the chatroom. 
     *   (hubPrefix + chatroomName + chatUsername + sessionNumber + sequence) is the full name of a chat message
     * @param observer The class that receives and displays chat messages.
     * @param face The face for broadcast sync and multicast chat interests.
     * @param keyChain The keychain to sign things with.
     * @param certificateName The name to locate the certificate.
     *
     * Constructor registers prefixes for both chat and broadcast namespaces.
     * This should be put into critical section, if the face is accessed by different threads
     */
	Chat
	  (const ndn::Name& broadcastPrefix,
	   const std::string& screenName, const std::string& chatRoom,
	   const ndn::Name& hubPrefix, ChatObserver *observer, ndn::Face& face, ndn::KeyChain& keyChain,
	   ndn::Name certificateName)
	  : screen_name_(screenName), chatroom_(chatRoom), maxmsgcachelength_(100),
		isRecoverySyncState_(true), sync_lifetime_(5000.0), observer_(observer),
		faceProcessor_(face), keyChain_(keyChain), certificateName_(certificateName),
		broadcastPrefix_(broadcastPrefix)
	{
	  chat_usrname_ = Chat::getRandomString();
	  chat_prefix_ = ndn::Name(hubPrefix).append(chatroom_).append(chat_usrname_);
	  
	  int session = (int)::round(getNowMilliseconds()  / 1000.0);
	  std::ostringstream tempStream;
	  tempStream << screen_name_ << session;
	  usrname_ = tempStream.str();
	  
	  sync_.reset(new ndn::ChronoSync2013
		(ndn::func_lib::bind(&Chat::sendInterest, this, _1, _2),
		 ndn::func_lib::bind(&Chat::initial, this), chat_prefix_,
		 broadcastPrefix_.append(chatroom_), session,
		 faceProcessor_, keyChain_, 
		 certificateName_, sync_lifetime_, onRegisterFailed));
	  
	  registeredPrefixId_ = faceProcessor_.registerPrefix
		(chat_prefix_, bind(&Chat::onInterest, this, _1, _2, _3, _4),
		 onRegisterFailed);   
	}
	
	~Chat()
	{
	}
	
	/**
	 * Sends a chat message.
	 * @param chatmsg The message to be sent.
	 *
	 * sendMessages calls sync publishNextSequenceNo, which expresses interest in broadcast namespace
     * This should be put into critical section, if the face is accessed by different threads
	 */
	void
	sendMessage(const std::string& chatmsg);

	/**
	 * Sends leave message. shutdown is not called in leave, therefore, you'll still be receiving and responding to sync interest.
	 *
	 * leave calls sync publishNextSequenceNo, which expresses interest in broadcast namespace
     * This should be put into critical section, if the face is accessed by different threads
	 */
	void
	leave();
    
    /**
     * When calling shutdown, destroy all pending interests and remove all
     * registered prefixes.
     * This accesses face, and should be in the same thread in which face is accessed.
     * shutdown differs from leave, the former should work with destructors, and unregister things;
     * while the latter just send another special message.
     */
    void
    shutdown()
    {
      // Stop receiving broadcast sync interests by calling sync_->shutdown, 
	  // which unregisters all prefixes from memoryContentCache of sync
	  sync_->shutdown();
	  faceProcessor_.removeRegisteredPrefix(registeredPrefixId_);
    }

  private:
	/**
	 * Use gettimeofday to return the current time in milliseconds.
	 * @return The current time in milliseconds since 1/1/1970, including fractions
	 * of a millisecond according to timeval.tv_usec.
	 */
	static ndn::MillisecondsSince1970
	getNowMilliseconds();

	int 
	notifyObserver(MessageTypes type, const char *prefix, const char *name, const char *msg, double timestamp);

	// Initialization: push the JOIN message in to the msgcache, update roster and start heartbeat.
	void
	initial();

	// Send a Chat Interest to fetch chat messages after get the user gets the Sync data packet back but will not send interest.
	void
	sendInterest
	  (const std::vector<ndn::ChronoSync2013::SyncState>& syncStates, bool isRecovery);

	// Send back Chat Data Packet which contains the user's message.
	void
	onInterest
	  (const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix,
	   const ndn::ptr_lib::shared_ptr<const ndn::Interest>& inst, ndn::Transport& transport,
	   uint64_t registeredPrefixId);

	// Processing the incoming Chat data.
	void
	onData
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& inst,
	   const ndn::ptr_lib::shared_ptr<ndn::Data>& co);

	void
	chatTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);

	/**
	 * This repeatedly calls itself after a timeout to send a heartbeat message
	 * (chat message type HELLO).
	 * This method has an "interest" argument because we use it as the onTimeout
	 * for Face.expressInterest.
	 */
	void
	heartbeat(const ndn::ptr_lib::shared_ptr<const ndn::Interest> &interest);

	/**
	 * This is called after a timeout to check if the user with prefix has a newer
	 * sequence number than the given temp_seq. If not, assume the user is idle and
	 * remove from the roster and print a leave message.
	 * This method has an "interest" argument because we use it as the onTimeout
	 * for Face.expressInterest.
	 */
	void
	alive
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest> &interest, int temp_seq,
	   const std::string& name, int session, const std::string& prefix);

	/**
	 * Append a new CachedMessage to msgcache, using given messageType and message,
	 * the sequence number from sync_->getSequenceNo() and the current time. Also
	 * remove elements from the front of the cache as needed to keep
	 * the size to maxmsgcachelength_.
	 */
	void
	messageCacheAppend(int messageType, const std::string& message);

	// Generate a random name for ChronoSync.
	static std::string
	getRandomString();

	static void
	onRegisterFailed(const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix);

	/**
	 * This is a do-nothing onData for using expressInterest for timeouts.
	 * This should never be called.
	 */
	static void
	dummyOnData
	  (const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
	   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
	
	class CachedMessage {
	public:
	  CachedMessage
		(int seqno, int msgtype, const std::string& msg, ndn::MillisecondsSince1970 time)
	  : seqno_(seqno), msgtype_(msgtype), msg_(msg), time_(time)
	  {}

	  int
	  getSequenceNo() const { return seqno_; }

	  int
	  getMessageType() const { return msgtype_; }

	  const std::string&
	  getMessage() const { return msg_; }

	  ndn::MillisecondsSince1970
	  getTime() const { return time_; }

	private:
	  int seqno_;
	  // This is really enum SyncDemo::ChatMessage_ChatMessageType, but make it
	  //   in int so that the head doesn't need to include the protobuf header.
	  int msgtype_;
	  std::string msg_;
	  ndn::MillisecondsSince1970 time_;
	};
	
	std::vector<ndn::ptr_lib::shared_ptr<CachedMessage> > msgcache_;
	std::vector<std::string> roster_;
	size_t maxmsgcachelength_;
	bool isRecoverySyncState_;
	
	std::string screen_name_;
	std::string chatroom_;
	std::string usrname_;
	
	ndn::Name broadcastPrefix_;
	
	// Added for comparison with name_t in sendInterest, not present in ndn-cpp
	std::string chat_usrname_;
	ndn::Name chat_prefix_;
	
	ndn::Milliseconds sync_lifetime_;
	ndn::ptr_lib::shared_ptr<ndn::ChronoSync2013> sync_;
	
	ndn::Face& faceProcessor_;
	ndn::KeyChain& keyChain_;
	ndn::Name certificateName_;
	
	ChatObserver *observer_;
	
	uint64_t registeredPrefixId_;
	
	// Added for not sending interest repeated for one piece of message
	std::map<std::string, int> syncTreeStatus_;
	
	const int prefixFromInstEnd_ = 4;
	const int prefixFromChatPrefixEnd_ = 2;
  };
}



#else // NDN_CPP_HAVE_PROTOBUF

#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
  cout <<
    "This program uses Protobuf but it is not installed. Install it and ./configure again." << endl;
}

#endif // NDN_CPP_HAVE_PROTOBUF

#endif // __ndnrtc__chrono__chat__