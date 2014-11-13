/**
 * Test-chrono-chat for ndnrtc add-on, separated from ndnrtc library.
 * Created by zhehao, based on Jeff T's ndn-cpp library and test-chrono-chat
 * Not following Yingdi's latest chrono-chat with digest-tree reset yet
 */

#include "chrono-chat.h"
#include "chatbuf.pb.h"
 
using namespace std;
using namespace ndn;
using namespace ndn::func_lib;

using namespace chrono_chat;

#if NDN_CPP_HAVE_STD_FUNCTION && NDN_CPP_WITH_STD_FUNCTION
// In the std library, the placeholders are in a different namespace than boost.
using namespace func_lib::placeholders;
#endif

void
Chat::initial()
{
	if (!enabled_)
		return;
		
	// Set the heartbeat timeout using the Interest timeout mechanism. The
	// heartbeat() function will call itself again after a timeout.
	// TODO: Are we sure using a "/timeout" interest is the best future call approach?
	Interest timeout("/timeout");
	timeout.setInterestLifetimeMilliseconds(60000);
	faceProcessor_.expressInterest(timeout, dummyOnData, bind(&Chat::heartbeat, shared_from_this(), _1));

	if (find(roster_.begin(), roster_.end(), usrname_) == roster_.end()) {
		roster_.push_back(usrname_);
		notifyObserver(MessageTypes::JOIN, chat_prefix_.getSubName
		  (0, chat_prefix_.size() - prefixFromChatPrefixEnd_).toUri().c_str(), screen_name_.c_str(), "", 0);
		
		messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_JOIN, "xxx");
	}
}

void
Chat::sendInterest
  (const vector<ChronoSync2013::SyncState>& syncStates, bool isRecovery)
{
	if (!enabled_)
		return ;
		
	// This is used by onData to decide whether to display the chat messages.
	isRecoverySyncState_ = isRecovery;
	
	vector<string> sendlist;
	vector<int> sessionlist;
	vector<int> seqlist;
	for (size_t j = 0; j < syncStates.size(); ++j) {
		Name name_components(syncStates[j].getDataPrefix());
		string name_t = name_components.get(-1).toEscapedString();
		int session = syncStates[j].getSessionNo();
		
		// bug in ndn-cpp's implementation
		if (name_t != chat_usrname_) {
			int index_n = -1;
			
			for (size_t k = 0; k < sendlist.size(); ++k) {
				if (sendlist[k] == syncStates[j].getDataPrefix()) {
					index_n = k;
					break;
				}
			}
			if (index_n != -1) {
				sessionlist[index_n] = session;
				seqlist[index_n] = syncStates[j].getSequenceNo();
			}
			else {
				sendlist.push_back(syncStates[j].getDataPrefix());
				sessionlist.push_back(session);
				seqlist.push_back(syncStates[j].getSequenceNo());
			}
		}
	}
	
	for (size_t i = 0; i < sendlist.size(); ++i) {
	
		ostringstream uri;
		uri << sendlist[i] << "/" << sessionlist[i];
		
		// Try to fetch all chat messages that wasn't fetched before
		int j = 0;
		std::map<string, int>::iterator item = syncTreeStatus_.find(uri.str());
  
		if (item == syncTreeStatus_.end()) {
			syncTreeStatus_.insert(std::pair<string, int>(uri.str(), 0));
			j = -1;
		}
		else {
			j = item->second;
		}
		
		for (j = j + 1; j <= seqlist[i]; j++)
		{
			ostringstream interestUri;
			interestUri << uri.str() << "/" << j;
			
			Interest interest(interestUri.str());
			interest.setInterestLifetimeMilliseconds(sync_lifetime_);
			faceProcessor_.expressInterest
			  (interest, bind(&Chat::onData, shared_from_this(), _1, _2),
			   bind(&Chat::chatTimeout, shared_from_this(), _1));
		}
		
		syncTreeStatus_[uri.str()] = seqlist[i];
	}
}

void
Chat::onInterest
  (const ptr_lib::shared_ptr<const Name>& prefix,
   const ptr_lib::shared_ptr<const Interest>& inst, Transport& transport,
   uint64_t registeredPrefixId)
{
	if (!enabled_)
		return ;
	SyncDemo::ChatMessage content;
	int seq = ::atoi(inst->getName().get(chat_prefix_.size() + 1).toEscapedString().c_str());
	for (int i = msgcache_.size() - 1; i >= 0; --i) {
		if (msgcache_[i]->getSequenceNo() == seq) {
			if (msgcache_[i]->getMessageType() != SyncDemo::ChatMessage_ChatMessageType_CHAT) {
				content.set_from(screen_name_);
				content.set_to(chatroom_);
				content.set_type((SyncDemo::ChatMessage_ChatMessageType)msgcache_[i]->getMessageType());
				content.set_timestamp(::round(msgcache_[i]->getTime() / 1000.0));
			}
			else {
				content.set_from(screen_name_);
				content.set_to(chatroom_);
				content.set_type((SyncDemo::ChatMessage_ChatMessageType)msgcache_[i]->getMessageType());
				content.set_data(msgcache_[i]->getMessage());
				content.set_timestamp(::round(msgcache_[i]->getTime() / 1000.0));
			}
			break;
		}
	}

	if (content.from().size() != 0) {
		ptr_lib::shared_ptr<vector<uint8_t> > array(new vector<uint8_t>(content.ByteSize()));
		content.SerializeToArray(&array->front(), array->size());
		Data co(inst->getName());
		co.setContent(Blob(array, false));
		keyChain_.sign(co, certificateName_);
		try {
			transport.send(*co.wireEncode());
		}
		catch (std::exception& e) {
		    // should probably notify with error
			cout << "Error sending the chat data " << e.what() << endl;
		}
	}
}

void
Chat::onData
  (const ptr_lib::shared_ptr<const Interest>& inst,
   const ptr_lib::shared_ptr<Data>& co)
{
	if (!enabled_)
		return ;
		
	SyncDemo::ChatMessage content;
	content.ParseFromArray(co->getContent().buf(), co->getContent().size());
	if (getNowMilliseconds() - content.timestamp() * 1000.0 < 120000.0) {
		string name = content.from();
		string prefix = co->getName().getPrefix(inst->getName().size() - prefixFromInstEnd_ + 2).toUri();
		int session = ::atoi(co->getName().get(inst->getName().size() - prefixFromInstEnd_ + 2).toEscapedString().c_str());
		int seqno = ::atoi(co->getName().get(inst->getName().size() - prefixFromInstEnd_ + 3).toEscapedString().c_str());
		
		cout << prefix << " " << session << " " << seqno << endl;
		
		ostringstream tempStream;
		tempStream << name << session;
		string nameAndSession = tempStream.str();

		size_t l = 0;
		//update roster
		while (l < roster_.size()) {
			string name_t2 = roster_[l].substr(0, roster_[l].size() - 10);
			int session_t = ::atoi(roster_[l].substr(roster_[l].size() - 10, 10).c_str());
			if (name != name_t2 && content.type() != 2)
				++l;
			else {
				if (name == name_t2 && session > session_t)
					roster_[l] = nameAndSession;
				break;
			}
		}
        if (l == roster_.size()) {
			roster_.push_back(nameAndSession);
			notifyObserver(MessageTypes::JOIN, inst->getName().getSubName
			  (0, inst->getName().size() - prefixFromInstEnd_).toUri().c_str(), name.c_str(), "", 0);
		}
        
		// Set the alive timeout using the Interest timeout mechanism.
		// TODO: Are we sure using a "/timeout" interest is the best future call approach?
		Interest timeout("/timeout");
		timeout.setInterestLifetimeMilliseconds(120000);
		faceProcessor_.expressInterest
		  (timeout, dummyOnData,
		   bind(&Chat::alive, shared_from_this(), _1, seqno, name, session, prefix));

		// isRecoverySyncState_ was set by sendInterest.
		// TODO: If isRecoverySyncState_ changed, this assumes that we won't get
		//   data from an interest sent before it changed.
		
		if (content.type() == 0 && content.from() != screen_name_) {
			notifyObserver(MessageTypes::CHAT,  inst->getName().getSubName
			  (0, inst->getName().size() - prefixFromInstEnd_).toUri().c_str(), content.from().c_str(), content.data().c_str(), 0);
		}
		else if (content.type() == 2) {
			// leave message
			vector<string>::iterator n = find(roster_.begin(), roster_.end(), nameAndSession);
			if (n != roster_.end() && name != screen_name_) {
				roster_.erase(n);
				notifyObserver(MessageTypes::LEAVE, inst->getName().getSubName
			      (0, inst->getName().size() - prefixFromInstEnd_).toUri().c_str(), name.c_str(),
			       "", 0);
			}
		}
	}
}

void
Chat::chatTimeout(const ptr_lib::shared_ptr<const Interest>& interest)
{
	// No chat data coming back.
	if (!enabled_)
		return ;
		
#ifdef CHAT_DEBUG
	cout << "Chat interest times out " << interest->getName().toUri() << endl;
#endif
}

void
Chat::heartbeat(const ptr_lib::shared_ptr<const Interest> &interest)
{
	if (!enabled_)
		return ;
		
	if (msgcache_.size() == 0)
		messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_JOIN, "xxx");

	sync_->publishNextSequenceNo();
	messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_HELLO, "xxx");

	// Call again.
	// TODO: Are we sure using a "/timeout" interest is the best future call approach?
	Interest timeout("/timeout");
	timeout.setInterestLifetimeMilliseconds(60000);
	faceProcessor_.expressInterest
	  (timeout, dummyOnData, bind(&Chat::heartbeat, shared_from_this(), _1));
}

void
Chat::sendMessage(const string& chatmsg)
{
	if (msgcache_.size() == 0)
		messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_JOIN, "xxx");

	// Ignore an empty message.
	// forming Sync Data Packet.
	if (chatmsg != "") {
		// This should be locked because it uses the same face.
		sync_->publishNextSequenceNo();
		
		messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_CHAT, chatmsg);
		notifyObserver(MessageTypes::CHAT, chat_prefix_.getSubName
		  (0, chat_prefix_.size() - prefixFromChatPrefixEnd_).toUri().c_str(), screen_name_.c_str(),
		   chatmsg.c_str(), 0);
	}
}

void
Chat::leave()
{
	sync_->publishNextSequenceNo();
	messageCacheAppend(SyncDemo::ChatMessage_ChatMessageType_LEAVE, "xxx");
}

void
Chat::alive
  (const ptr_lib::shared_ptr<const Interest> &interest, int temp_seq,
   const string& name, int session, const string& prefix)
{
	if (!enabled_)
		return ;
		
	int seq = sync_->getProducerSequenceNo(prefix, session);
	ostringstream tempStream;
	tempStream << name << session;
	string nameAndSession = tempStream.str();
	vector<string>::iterator n = find(roster_.begin(), roster_.end(), nameAndSession);
	if (seq != -1 && n != roster_.end()) {
		if (temp_seq == seq){
			roster_.erase(n);
			notifyObserver(MessageTypes::LEAVE, interest->getName().getSubName
			  (0, interest->getName().size() - prefixFromInstEnd_).toUri().c_str(), name.c_str(), "", 0);
		}
	}
}

void
Chat::messageCacheAppend(int messageType, const string& message)
{
	msgcache_.push_back(ptr_lib::make_shared<CachedMessage>
	  (sync_->getSequenceNo(), messageType, message, getNowMilliseconds()));
	while (msgcache_.size() > maxmsgcachelength_)
		msgcache_.erase(msgcache_.begin());
}

string
Chat::getRandomString()
{
	string seed("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789");
	string result;
	uint8_t random[10];
	RAND_bytes(random, sizeof(random));
	for (int i = 0; i < 10; ++i) {
		// Using % means the distribution isn't uniform, but that's OK.
		size_t pos = (size_t)random[i] % seed.size();
		result += seed[pos];
	}

	return result;
}

void
Chat::onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix)
{
	cout << "Register failed for prefix " << prefix->toUri() << endl;
}

MillisecondsSince1970
Chat::getNowMilliseconds()
{
	struct timeval t;
	// Note: configure.ac requires gettimeofday.
	gettimeofday(&t, 0);
	return t.tv_sec * 1000.0 + t.tv_usec / 1000.0;
}

void
Chat::dummyOnData
(const ptr_lib::shared_ptr<const Interest>& interest,
 const ptr_lib::shared_ptr<Data>& data)
{

}

int 
Chat::notifyObserver(MessageTypes type, const char *prefix, const char *name, const char *msg, double timestamp)
{
  if (observer_)
	observer_->onStateChanged(type, prefix, name, msg, timestamp);
  else {
	string state = "";
	switch (type) {
	  case (MessageTypes::JOIN):	state = "Join"; break;
	  case (MessageTypes::CHAT):	state = "Chat"; break;
	  case (MessageTypes::LEAVE):	state = "Leave"; break;
	}
	cout << state << "\t" << timestamp << " " << name << " : " << msg << endl;
  }
  return 1;
}

static const char *WHITESPACE_CHARS = " \n\r\t";

/**
* Modify str in place to erase whitespace on the left.
* @param str
*/
static inline void
trimLeft(string& str)
{
	size_t found = str.find_first_not_of(WHITESPACE_CHARS);
	if (found != string::npos) {
		if (found > 0)
			str.erase(0, found);
	}
	else
		// All whitespace
		str.clear();
}

/**
* Modify str in place to erase whitespace on the right.
* @param str
*/
static inline void
trimRight(string& str)
{
	size_t found = str.find_last_not_of(WHITESPACE_CHARS);
	if (found != string::npos) {
		if (found + 1 < str.size())
			str.erase(found + 1);
	}
	else
	// All whitespace
		str.clear();
}

/**
* Modify str in place to erase whitespace on the left and right.
* @param str
*/
static void
trim(string& str)
{
	trimLeft(str);
	trimRight(str);
}

static void
onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix)
{
	cout << "Register failed for prefix " << prefix->toUri() << endl;
}