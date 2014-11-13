#include "external-observer.h"
#include "chrono-chat.h"
#include "conference-discovery-sync.h"

#include <exception>
#include <openssl/rand.h>
#include <stdio.h>

using namespace chrono_chat;
using namespace conference_discovery;
using namespace std;
using namespace ndn;

//ptr_lib::shared_ptr<Chat> chat;

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

/**
 * Read a line from from stdin and return a trimmed string.  (We don't use
 * cin because it ignores a blank line.)
 */
static string
stdinReadLine()
{
  char inputBuffer[1000];
  ssize_t nBytes = ::read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1);
  if (nBytes < 0)
    // Don't expect an error reading from stdin.
    throw runtime_error("stdinReadLine: error reading from STDIN_FILENO");

  inputBuffer[nBytes] = 0;
  string input(inputBuffer);
  trim(input);

  return input;
}

/**
 * Poll stdin and return true if it is ready to ready (e.g. from stdinReadLine).
 */
static bool
isStdinReady()
{
  struct pollfd pollInfo;
  pollInfo.fd = STDIN_FILENO;
  pollInfo.events = POLLIN;

  return poll(&pollInfo, 1, 0) > 0;
}

static string
getRandomString()
{
  string seed("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789");
  string result;
  uint8_t random[10];
  RAND_bytes(random, sizeof(random));
  for (int i = 0; i < 3; ++i) {
    // Using % means the distribution isn't uniform, but that's OK.
    size_t pos = (size_t)random[i] % seed.size();
    result += seed[pos];
  }

  return result;
}

class SampleChatObserver : public ChatObserver
{
  public:
    SampleChatObserver()
    {

    }

    void onStateChanged(chrono_chat::MessageTypes type, const char *prefix, const char *userName, const char *msg, double timestamp)
    {
      string state = "";
          switch (type) {
                case (chrono_chat::MessageTypes::JOIN): state = "Join"; break;
                case (chrono_chat::MessageTypes::CHAT): state = "Chat"; break;
                case (chrono_chat::MessageTypes::LEAVE):        state = "Leave"; break;
          }
          if (msg[0] != 0) {
            cout << state << " " << timestamp << "\t" << userName << " : " << msg << endl;
          }
          else {
            cout << state << " " << timestamp << "\t" << userName << endl;
          }
    }

    ptr_lib::shared_ptr<Chat> chat;
};

/**
 * Test for large number of registration in a row.
 */
/*
class Echo {
public:
  Echo(KeyChain &keyChain, const Name& certificateName)
  : keyChain_(keyChain), certificateName_(certificateName), responseCount_(0)
  {
  }

  // onInterest.
  void operator()
     (const ptr_lib::shared_ptr<const Name>& prefix,
      const ptr_lib::shared_ptr<const Interest>& interest, Transport& transport,
      uint64_t registeredPrefixId)
  {
    ++responseCount_;

    // Make and sign a Data packet.
    Data data(interest->getName());
    string content(string("Echo ") + interest->getName().toUri());
    data.setContent((const uint8_t *)&content[0], content.size());
    keyChain_.sign(data, certificateName_);
    Blob encodedData = data.wireEncode();

    cout << "Sent content " << content << endl;
    transport.send(*encodedData);
  }

  // onRegisterFailed.
  void operator()(const ptr_lib::shared_ptr<const Name>& prefix)
  {
    ++responseCount_;
    cout << "Register failed for prefix " << prefix->toUri() << endl;
  }

  KeyChain keyChain_;
  Name certificateName_;
  int responseCount_;
};
 
int main()
{
  int nPrefix = 100;
  
  Face face;
  KeyChain keyChain;
  Name certificateName = keyChain.getDefaultCertificateName();
  face.setCommandSigningInfo(keyChain, certificateName);
  
  Name prefix("/ndn/edu/ucla/remap");
  Echo echo(keyChain, certificateName);
  
  for (int i = 0; i < nPrefix; i++) {
    ostringstream ss;
    ss << "prefix" << i;
    Name name(prefix);
    name.append(ss.str());
    face.registerPrefix(name, func_lib::ref(echo), func_lib::ref(echo));
    
    cout << name.toUri() << endl;
    usleep(10000);
  }
  while (1) {
	face.processEvents();
	// We need to sleep for a few milliseconds so we don't use 100% of the CPU.
	usleep(100);
  }
}
*/

/**
 * Test function for chrono-chat
 */
int main()
{
  string screenName = getRandomString();
  string chatroom = "ndnchat";
  
  cout << "Please input hub prefix[/ndn/edu/ucla/remap]: " << endl;
  string hubPrefixString = "/ndn/edu/ucla/remap";
  hubPrefixString = stdinReadLine();
  if (hubPrefixString == "") {
    hubPrefixString = "/ndn/edu/ucla/remap";
  }
  
  Name hubPrefix(hubPrefixString);
  Name chatBroadcastPrefix("/ndn/broadcast/chrono-chat/");

  Face face;
  // Using default keyChain in ndn-cpp
  KeyChain keyChain;
  Name certificateName = keyChain.getDefaultCertificateName();

  face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());
  std::vector<SampleChatObserver*> observers;
  // SampleChatObserver chatObserver;

  // This throws an libc++abi.dylib: terminate called throwing an exception
  // which is caused by "socket cannot connect to socket"
  int nChats = 1;
  try {
    // Instruction for using the class at chrono-chat.h
    for (int i = 0; i < nChats; i++) {
      observers.push_back(new SampleChatObserver());

      std::stringstream ss;
      ss << chatroom << i;

      observers[i]->chat.reset
        (new Chat(chatBroadcastPrefix, screenName, ss.str(),
           hubPrefix, observers[i],
           face, keyChain, certificateName));

        observers[i]->chat->start();
      usleep(500000);
    }
  }
  catch (std::exception& e) {
    cout << e.what() << '\n';
  }

  std::string msgString = "";
  
  while (1) {
	if (isStdinReady())
	{
	  msgString = stdinReadLine();
	  // resets chat without doing anything else, to test Peter's callback problem
	  if (msgString == "-reset") {
			  cout << "Chat deleted." << endl;
			  observers[0]->chat->shutdown();
			  observers[0]->chat.reset();
			  continue;
	  }
	  if (msgString == "-leave" || msgString == "-exit") {
			break;
	  }
	  if (msgString == "-auto") {
			for (int i = 0; i < 30; i++) {
			  ostringstream ss;
			  ss << i;
			  observers[0]->chat->sendMessage(ss.str());
			  for (int j = 0; j < 100; j++) {
					face.processEvents();
					usleep(2000);
			  }
			}
			continue;
	  }

	  observers[0]->chat->sendMessage(msgString);
	}

	face.processEvents();
	usleep(10000);
  }
  // chatObserver.chat->leave();

  int sleepSeconds = 0;
  while (sleepSeconds < 1000000) {
        face.processEvents();
        usleep(10000);
        sleepSeconds += 10000;
  }

  return 1;
}