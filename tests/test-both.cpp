#include "test-both.h"
#include <exception>
#include <openssl/rand.h>
#include <stdio.h>

using namespace chrono_chat;
using namespace conference_discovery;
using namespace std;

using namespace test;

ptr_lib::shared_ptr<Chat> chat;
ptr_lib::shared_ptr<ConferenceDiscovery> discovery;

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

/**
 * Test function for chrono-chat and conference discovery
 */
int main()
{
  string screenName = getRandomString();
  string chatroom = "ndnchat";
  
  Name hubPrefix("/ndn/edu/ucla/remap/ndnrtc");
  Name chatBroadcastPrefix("/ndn/broadcast/chrono-chat/");
  std::string conferenceDiscoveryBdcastPrefix = "/ndn/broadcast/ndnrtc/conferences";
  
  Face face;
  // Using default keyChain in ndn-cpp
  KeyChain keyChain;
  Name certificateName = keyChain.getDefaultCertificateName();
  
  face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());
  
  // This throws an libc++abi.dylib: terminate called throwing an exception
  // which is caused by "socket cannot connect to socket"
  try {
    chat.reset
      (new Chat(chatBroadcastPrefix, screenName, chatroom,
  	   hubPrefix, NULL, face, keyChain, certificateName));
  	chat->start();
  	
  	ConferenceDescription cd;
  	ConferenceInfoFactory factory(ptr_lib::make_shared<ConferenceDescription>(cd));

  	discovery.reset
  	  (new ConferenceDiscovery(conferenceDiscoveryBdcastPrefix, 
  	   NULL, ptr_lib::make_shared<ConferenceInfoFactory>(factory), 
  	   face, keyChain, certificateName));
  	   
  	//ConferenceDescription thisConference;
  	//thisConference.setDescription("conference: " + screenName);
  	//discovery->publishConference
  	//  (screenName, hubPrefix, ptr_lib::make_shared<ConferenceDescription>(thisConference));
  }
  catch (std::exception& e) {
    cout << e.what() << '\n';
  }
  
  std::string msgString = "";
  while (1) {
    // If using stdin as input, and stdout as output; 
    // Output buffering could mess with this: all the msgs could be displayed after
    // leaving.
    if (isStdinReady())
    {
      msgString = stdinReadLine();
      if (msgString == "-leave" || msgString == "-exit") {
        break;
      }
      if (msgString.find("-stop ") != std::string::npos) {
        cout << "Using default prefix: " << hubPrefix.toUri() << endl;
        discovery->stopPublishingConference(msgString.substr(string("-stop ").size()), hubPrefix);
        continue;
      }
      if (msgString.find("-start ") != std::string::npos) {
        cout << "Using default prefix: " << hubPrefix.toUri() << endl;
        
        ConferenceDescription thisConference;
    	thisConference.setDescription("conference: " + msgString);
        discovery->publishConference
          (msgString.substr(string("-start ").size()), hubPrefix, ptr_lib::make_shared<ConferenceDescription>(thisConference));
        continue;
      }
      if (msgString.find("-show ") != std::string::npos) {
        ptr_lib::shared_ptr<ConferenceDescription> description = 
          ptr_lib::dynamic_pointer_cast<ConferenceDescription>
            (discovery->getConference(msgString.substr(string("-show ").size())));
        
        if (description) {
          cout << description->getDescription() << endl;
        }
        continue;
      }
      chat->sendMessage(msgString);
    }
    try {
      face.processEvents();
    }
    catch (std::exception& e) {
      cout << e.what() << endl;
    }
    usleep(10000);
  }
  chat->leave();
  
  int sleepSeconds = 0;
  while (sleepSeconds < 1000000) {
	face.processEvents();
	usleep(10000);
	sleepSeconds += 10000;
  }
  return 1;
}