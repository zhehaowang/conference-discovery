#include "test.h"
#include <exception>
#include <openssl/rand.h>

using namespace chrono_chat;
using namespace std;

ptr_lib::shared_ptr<Chat> chat;
ptr_lib::shared_ptr<ConferenceDiscovery> discovery;

static const char *WHITESPACE_CHARS = " \n\r\t";

int
Test::startChronoChat()
{
  return 1;
}

int 
Test::stopChronoChat()
{
  return 1;
}

int 
Test::startDiscovery()
{
  return 1;
}

int 
Test::stopDiscovery()
{
  return 1;
}

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
  
  Name hubPrefix("/ndn/edu/ucla/remap");
  Name chatBroadcastPrefix("/ndn/broadcast/chrono-chat0.3/");
  std::string conferenceDiscoveryBdcastPrefix = "/ndn/broadcast/discovery";
  Observer observer;
  
  Face face;
  // Using default keyChain in ndn-cpp
  KeyChain keyChain;
  Name certificateName = keyChain.getDefaultCertificateName();
  
  face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());
  
  // This throws an libc++abi.dylib: terminate called throwing an exception
  // which is caused by "socket cannot connect to socket"
  try {
    chat.reset(new Chat(chatBroadcastPrefix, screenName, chatroom,
  	   hubPrefix, NULL, face, keyChain, certificateName)); 
  	discovery.reset(new ConferenceDiscovery(conferenceDiscoveryBdcastPrefix, 
  	   NULL, face, keyChain, certificateName));
  	discovery->publishConference(screenName, hubPrefix);
  }
  catch (std::exception& e) {
    cout << e.what() << '\n';
  }
  
  // This example is not thread-safe; 
  // And it's not so good as the polling approach in Jeff T's
  // test-chrono-chat, using this only for test for now.
  //pthread_t chatThread;
  //pthread_create(&chatThread, NULL, readChatMsg, NULL);
  
  std::string msgString = "";
  while (1) {
    // If using stdin as input, and stdout as output; 
    // Output buffering could mess with this: all the msgs could be displayed after
    // leaving.
    if (isStdinReady())
    {
      msgString = stdinReadLine();
      if (msgString == "leave" || msgString == "exit")
        // We will send the leave message below.
        break;
    
      chat->sendMessage(msgString);
    }
    face.processEvents();
    usleep(10000);
  }
  chat->leave();
  
  return 1;
}