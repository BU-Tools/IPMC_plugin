#include <signal.h> 
#include <ipmiconsole.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <stdlib.h>
#include <string>
//#include <readline/readline.h>

#include <fcntl.h>

#define MAXBYTESTOREAD 10000
#define MAXBYTESTOWRITE 65

#define RD_ADDR 0xB
#define RD_AVAILABLE 0x1000

#define WR_ADDR 0xA
#define WR_HALF_FULLL 0x2000
#define WR_FULL 0x4000
#define WR_FULL_FULL (WR_HALF_FULL | WR_FULL)

#define ACK 0x100

bool loop;
void signal_handler(int sig){
  if(SIGINT == sig){
    loop = false;
  }
}


void configpselect(fd_set * const set, timespec * tstruct, int seconds) {
  // Zero out 
  FD_ZERO(set);
  // timeout to read
  tstruct->tv_sec = seconds;
  tstruct->tv_nsec = 0;
}

void configipmi(ipmiconsole_ipmi_config * const ipmiUser) {   
  ipmiUser->username = (char *)"soluser";
  ipmiUser->password = (char *)"solpasswd";
  ipmiUser->k_g = (unsigned char*)"root";
  ipmiUser->k_g_len = 5;
  ipmiUser->privilege_level = IPMICONSOLE_PRIVILEGE_USER;
  ipmiUser->cipher_suite_id = 0;
  ipmiUser->workaround_flags = IPMICONSOLE_WORKAROUND_DEFAULT;
}

void configprotocol(ipmiconsole_protocol_config * const protConf) {
  // Default is 60 seconds. Other default values can be found in ipmiconsole.h
  protConf->session_timeout_len = -1;
  protConf->retransmission_timeout_len = -1;
  protConf->retransmission_backoff_count = -1;
  protConf->keepalive_timeout_len = -1;
  protConf->retransmission_keepalive_timeout_len = -1;
  protConf->acceptable_packet_errors_count = -1;
  protConf->maximum_retransmission_count = -1;

}

void configEngine(ipmiconsole_engine_config * engine) {
  engine->engine_flags = IPMICONSOLE_ENGINE_OUTPUT_ON_SOL_ESTABLISHED;
  engine->behavior_flags = IPMICONSOLE_BEHAVIOR_DEFAULT;
  engine->debug_flags = IPMICONSOLE_DEBUG_DEFAULT;
  // STDERR is very useful
  //  ipmiEngine.debug_flags = IPMICONSOLE_DEBUG_STDERR;
}
//
//ool SetNonBlocking(int &fd, bool value) {
// // Get the previous flags
// int currentFlags = fcntl(fd, F_GETFL, 0);
// if(currentFlags < 0) {return false;}
//
// // Make the socket non-blocking
// if(value) {
//   currentFlags |= O_NONBLOCK;
// } else {
//   currentFlags &= ~O_NONBLOCK;
// }
//
// int currentFlags2 = fcntl(fd, F_SETFL, currentFlags);
// if(currentFlags2 < 0) {return false;}
//
// return(true);
//

int main() {

  // For pselect ----------
  // Set of file descriptors to read from. If our fd was 9, there would be 10 elements, nine 0s in the beginning and one 1 at the end. Checkkk
  fd_set readSet;
  // Set time out for reading.
  struct timespec timeoutStruct;
  configpselect(&readSet, &timeoutStruct, 5);

  fd_set ipmiSet;
  configpselect(&ipmiSet, &timeoutStruct, 5);
  fd_set commandSet;
  configpselect(&commandSet, &timeoutStruct, 5);
  // ----------------------

  // configure ipmi
  struct ipmiconsole_ipmi_config ipmiUserInfo;
  configipmi(&ipmiUserInfo);
  
  // configure protocol
  struct ipmiconsole_protocol_config protocolConfiguration;
  configprotocol(&protocolConfiguration);
  
  // configure engine
  struct ipmiconsole_engine_config ipmiEngine;
  configEngine(&ipmiEngine);
  
  // Initialize engine. Returns 0 on success and 0 for the first argument defaults to 4
  ipmiconsole_engine_init(0, IPMICONSOLE_DEBUG_STDERR);
  
  // Create a context (you get a file descriptor from the context)
  ipmiconsole_ctx_t ipmiContext = ipmiconsole_ctx_create("192.168.20.56", &ipmiUserInfo, &protocolConfiguration, &ipmiEngine);
   
  // SOL payload instance number. Most systems support only a single instance. Some systems allow users to access multiple (meaning multiple users could then see the same serial session, for example)
  int SOL_PAYLOAD_NUM = 1;
  // Configure context
  ipmiconsole_ctx_set_config(ipmiContext, IPMICONSOLE_CTX_CONFIG_OPTION_SOL_PAYLOAD_INSTANCE, (void *)&SOL_PAYLOAD_NUM);
  
  // Submit context to the engine
  // returns 0 on success. No callback function (first NULL) or parameter (second NULL)
  ipmiconsole_engine_submit(ipmiContext, NULL, NULL);

  // Get a file descriptor
  int fd = ipmiconsole_ctx_fd(ipmiContext);

  // Put the file descriptor in the set of descriptors to be read from
  FD_SET(fd, &readSet);
  FD_SET(fd, &ipmiSet);
  
  int timeout = 5;
  int timer = 0;
  while(3 != ipmiconsole_ctx_status(ipmiContext)) {
    if(timeout == timer) {
      printf("Trying to establish connection timed out in %d seconds\n", timeout);
      return 0;
    }
    usleep(1000000);
    timer++;
    printf("Status in loop: %d\n", ipmiconsole_ctx_status(ipmiContext));
  }
  
  printf("Success! Context is now: %d\n", ipmiconsole_ctx_status(ipmiContext));

  int commandfd = 0; // stdin
  FD_SET(commandfd, &readSet);
  FD_SET(commandfd, &commandSet);
  char writeByte;
    
  char readChar;

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigaction(SIGINT, &sa, NULL);
  //  signal(SIGINT,signal_handler);
  loop = true;
  
  while(loop) {
    fd_set readSetCopy = readSet; 
    // no timeout. We don't check max(fd, commandfd) because commandfd is 0
    if(pselect((fd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL) > 0){
    
    if(FD_ISSET(fd, &readSetCopy)) {
      // read everything
//      while(true) {
//	fd_set ipmiSetCopy = ipmiSet;
//	if(0 == pselect((fd + 1), &ipmiSetCopy, NULL, NULL, &timeoutStruct, NULL)) {printf("breaking"); break;}
      read(fd, &readChar, sizeof(readChar));
      printf("%c", readChar);
	//    }
    }

    // Write one byte
    if(FD_ISSET(commandfd, &readSetCopy)) {
      // This would probably never not be true
      if(0 < read(commandfd, &writeByte, sizeof(writeByte))) {
	// Catch group separator (not 62)
	if(62 == writeByte) {
	  // Closing file descriptor
	  ipmiconsole_ctx_destroy(ipmiContext);
	  
	  printf("Closing SOL session ...\n");
	  // Non zero parameter will cause SOL sessions to be torn down cleanly.
	  // Will block until all active ipmi sessions cleanly closed or timed out
	  ipmiconsole_engine_teardown(1);
	  printf("SOL session successfully terminated\n");
	  return 0;
	}
	write(fd, &writeByte, sizeof(writeByte));
      }
    }
    }
  }
//
//  // Reads initial output of zynq
//  while(true) {
//    fd_set readSetCopy = readSet;
//    // Currently not sure when to stop reading so pselect is set to timeout
//    if(0 == pselect((fd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL)) {break};
//    read(fd, &readChar, sizeof(readChar));
//    printf("%c", readChar);
//  }
//  
//  char lastCharRead;
//  char* command;
//  std::string quitCommand = "quit";
//  
//  // readline blocks until user types in command
//  while((command = readline("")) != NULL) {
//    std::size_t msgLen = 0;
//    std::string msg = command;
//    // Check if user wants to quit
//    if(std::string::npos != msg.find(quitCommand)) {
//      // Closing file descriptor
//      ipmiconsole_ctx_destroy(ipmiContext);
//      
//      printf("Closing SOL session ...\n");
//      // Non zero parameter will cause SOL sessions to be torn down cleanly.
//      // Will block until all active ipmi sessions cleanly closed or timed out
//      ipmiconsole_engine_teardown(1);
//      printf("SOL session successfully terminated\n");
//      return 0;
//    }
//    
//    msg.push_back('\n');
//
//    // 65 is currently an arbitrary command length limit
//    char cMessage[MAXBYTESTOWRITE];
//
//    // Copy all of msg excluding NULL 
//    msgLen = msg.copy(cMessage, msg.length(), 0);
//    write(fd, cMessage, msgLen);
//    
//    std::string readString = "";
//    
//    // Reset lastCharRead to NULL
//    lastCharRead = 0;
//    
//    while(1) {
//      fd_set readSetCopy = readSet;
//      // Currently not sure when to stop reading so pselect is set to timeout
//      if(0 == pselect((fd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL)) {break};
//      read(fd, &readChar, sizeof(readChar));
//      readString.push_back(readChar);
//      if('o' == readChar && 'g' == lastCharRead) {break};
//      lastCharRead = readChar;
//    }
//
//    // readline mallocs a new buffer every time
//    free(command);
//    if('o' == readChar && 'g' == lastCharRead) {break};
//    printf("Received:\n%s", readString.c_str());
//  }
//
//  // Reads output after "go"
//  while(true) {
//    fd_set readSetCopy = readSet;
//    // Currently not sure when to stop reading so pselect is set to timeout in 1
//    if(0 == pselect((fd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL)) {break};
//    read(fd, &readChar, sizeof(readChar));
//    printf("%c", readChar);
//  }
//  
//  // readline blocks until user types in command
//  while((command = readline("")) != NULL) {
//    std::size_t msgLen = 0;
//    std::string msg = command;
//    // Check if user wants to quit
//    if(std::string::npos != msg.find(quitCommand)) break;
//    msg.push_back('\n');
//
//    // 65 is currently an arbitrary command length limit
//    char cMessage[MAXBYTESTOWRITE];
//
//    // Copy all of msg excluding NULL 
//    msgLen = msg.copy(cMessage, msg.length(), 0);
//    write(fd, cMessage, msgLen);
//    
//    std::string readString = "";
//    
//    // Reset lastCharRead to NULL
//    lastCharRead = 0;
//    
//    // 10000: current arbitrarily chosen limit on number of bytes we're willing to read
//    for(int i = 0; i < MAXBYTESTOREAD; i++) {
//      fd_set readSetCopy = readSet;
//      read(fd, &readChar, sizeof(readChar));
//      readString.push_back(readChar);
//      if(' ' == readChar && '#' == lastCharRead) break;
//      lastCharRead = readChar;
//    }
//
//    // readline mallocs a new buffer every time
//    free(command);
//    printf("Received:\n%s", readString.c_str());
//  }

  // Closing file descriptor
  ipmiconsole_ctx_destroy(ipmiContext);

  printf("Closing SOL session ...\n");
  // Non zero parameter will cause SOL sessions to be torn down cleanly.
  // Will block until all active ipmi sessions cleanly closed or timed out
  ipmiconsole_engine_teardown(1);
  printf("SOL session successfully terminated\n");
  
  return 0;
}
