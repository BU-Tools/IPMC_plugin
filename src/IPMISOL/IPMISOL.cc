#include <IPMISOL/IPMISOL.hh>
#include <string.h> // strlen
#include <unistd.h>
#include <IPMISOL_Exceptions/IPMISOL_Exceptions.hh>
#include <signal.h>
#include <ncurses.h>
//#include <termios.h>

#define SOLUSERNAME "soluser"
#define SOLPASSWORD "solpasswd"
#define K_G "root"

// SOL payload instance number. Most systems support only a single instance. Some systems allow users to access multiple (meaning multiple users could then see the same serial session, for example)
#define SOL_PAYLOAD_NUM 1

#define STDIN 0

// check if the version is less than 1.2.2, i.e whether it has ipmiconsole_ctx_set_config()
#if FREEIPMI_PACKAGE_VERSION_MAJOR == 0 || (FREEIPMI_PACKAGE_VERSION_MAJOR == 1 && FREEIPMI_PACKAGE_VERSION_MINOR < 2) || ( FREEIPMI_PACKAGE_VERSION_MAJOR == 1 && FREEIPMI_PACKAGE_VERSION_MINOR == 2 && FREEIPMI_PACKAGE_VERSION_PATCH < 2 )
#define VERSION_TOO_OLD
#endif

// Constructor
IPMISOL::IPMISOL(std::string const & _ipmc_ip_addr):
  solfd(-1) {
  Connect(_ipmc_ip_addr);
}

// Deconstructor
IPMISOL::~IPMISOL() {
  // shutdown will check if a proper connection was established and if so, terminate it
  shutdown();
}

// A do-it-all function to set up the SOL connection
void IPMISOL::Connect(std::string const & _ip) {
  // Always initialize non-const file descriptors as invalid
  //solfd = -1;
  //  commandfd = 0;

  // zero out set of file descriptors
  FD_ZERO(&readSet);
  
  // set pselect timeout to 5 seconds
  timeoutStruct.tv_sec = 5;
  timeoutStruct.tv_nsec = 0;

  // configure user info
  configUser(&ipmiUser);
  // configure protocol
  configProtocol(&ipmiProtocol);
  // configure engine
  configEngine(&ipmiEngine);

  // Initialize engine. Returns 0 on success. 0 for first argument defaults to 4 threads
  ipmiconsole_engine_init(0, IPMICONSOLE_DEBUG_STDERR);

  ipmc_ip_addr = _ip;

  // define context
  ipmiconsole_ctx_t ipmiContext = ipmiconsole_ctx_create(ipmc_ip_addr.c_str(), &ipmiUser, &ipmiProtocol, &ipmiEngine);

  // ipmiconsole_ctx_create returns NULL on error and sets errno. Use perror to print 
  if(NULL == ipmiContext) {
    perror("Error");
    BUException::CONNECT_ERROR e;
    e.Append(ipmiconsole_ctx_errormsg(ipmiContext));
    throw e;
  }

  // Configure context if version is up to date 
  #ifndef VERSION_TOO_OLD
  // Unfortunately &SOL_PAYLOAD_NUM does not work
  int num = SOL_PAYLOAD_NUM;
  ipmiconsole_ctx_set_config(ipmiContext, IPMICONSOLE_CTX_CONFIG_OPTION_SOL_PAYLOAD_INSTANCE, (void *)&num);
  #endif

  // Submit context to engine
  // returns 0 on success. No callback function (2nd arg) or callback parameters (3rd arg)
  ipmiconsole_engine_submit(ipmiContext, NULL, NULL);

  // get file descriptor to talk to and read from
  solfd = ipmiconsole_ctx_fd(ipmiContext);
  
  // Put file descrptor in set of descriptors to read from
  FD_SET(solfd, &readSet);
  FD_SET(STDIN, &readSet);
  
  int timeout = 5;
  int timer = 0;
  while(3 != ipmiconsole_ctx_status(ipmiContext)) {
    if(timeout == timer) {
      BUException::CONNECT_ERROR e;
      e.Append("Trying to establish ipmi SOL connection timed out\n");
      //      printf("Trying to establish connection timed out in %d seconds\n", timeout);
      throw e;
    }
    usleep(1000000);
    timer++;
    printf("Connection status: %d\n", ipmiconsole_ctx_status(ipmiContext));
  }
}

void IPMISOL::configUser(ipmiconsole_ipmi_config * const user) {
  user->username = (char *)SOLUSERNAME;
  user->password = (char *)SOLPASSWORD;
  user->k_g = (unsigned char *)K_G;
  user->k_g_len = strlen(K_G) + 1;
  user->privilege_level = IPMICONSOLE_PRIVILEGE_USER;
  user->cipher_suite_id = 0;
  user->workaround_flags = IPMICONSOLE_WORKAROUND_DEFAULT;
}

void IPMISOL::configProtocol(ipmiconsole_protocol_config * const prot) {
  prot->session_timeout_len = -1;
  prot->retransmission_timeout_len = -1;
  prot->retransmission_backoff_count = -1;
  prot->keepalive_timeout_len = -1;
  prot->retransmission_keepalive_timeout_len = -1;
  prot->acceptable_packet_errors_count = -1;
  prot->maximum_retransmission_count = -1;
}

void IPMISOL::configEngine(ipmiconsole_engine_config * const engine) {
  engine->engine_flags = IPMICONSOLE_ENGINE_DEFAULT;
  //engine->engine_flags = IPMICONSOLE_ENGINE_OUTPUT_ON_SOL_ESTABLISHED;
  engine->behavior_flags = IPMICONSOLE_BEHAVIOR_DEFAULT;
  engine->debug_flags = IPMICONSOLE_DEBUG_DEFAULT;
  // STDERR is very useful and very verbose
  //engine->debug_flags = IPMICONSOLE_DEBUG_STDERR;
}

// For Ctrl-C handling
//---------------------------------------------------------------------------  
bool volatile interactiveLoop;

//void IPMISOL::signal_handler(int const signum) {
void static signal_handler(int const signum) {
  if(SIGINT == signum) {
    printf("\n");
    interactiveLoop = false;
  }
  return;
}

// To catch Ctrl-C and break out of talking through SOL
struct sigaction sa;
// To restore old handling of Ctrl-C
struct sigaction oldsa;
//---------------------------------------------------------------------------  

// The function where all the talking and reading through SOL happens
void IPMISOL::SOLConsole() {
  char readByte;
  char writeByte;

  // Instantiate sigaction struct member with signal handler function 
  sa.sa_handler = signal_handler;
  // Catch SIGINT and pass to function signal_handler within sigaction struct sa
  sigaction(SIGINT, &sa, &oldsa);
  interactiveLoop = true;

  printf("Opening SOL comm ...\n");
  printf("Press Ctrl-] to close\n");

  //  struct termios config;                                                                         
  //  tcgetattr(0, &config);                                                                         
  //  config.c_lflag &= ~ICANON;                                                                     
  //  config.c_cc[VMIN] = 1;                                                                         
  //  tcsetattr(0, TCSANOW, &config);                                                                
  
  // Enable curses mode. This call is necessary before calling other ncurses functions such as cbreak(). In BUTOOL, for some reason, if only initscr() is called but cbreak() and noecho() are not, the first call to SOLConsole will act the way we want, but subsequent calls result in very weird behavior such as not recognizing the backspace key. The functionality we want with ncurses is to read characters being typed (VMIN = 1). I believe the default settings in initscr() accomplishes this. We call cbreak() to make sure we know what settings we are operating in (cbreak() also sets VMIN = 1).
  initscr();
  // raw() only allows Ctrl-] to break out of the loop. cbreak() allows both Ctrl-] and Ctrl-C to break.
  // Although noecho() is called after cbreak(), I believe it is also called inside cbreak(). Source: https://utcc.utoronto.ca/~cks/space/blog/unix/CBreakAndRaw The primary reason for using cbreak() is to set VMIN = 1 as explained above.
  cbreak();
  noecho();

  while(interactiveLoop) {
    // Make a copy every time readSet is passed to pselect because pselect changes contents of fd_sets and we don't want readSet to change
    fd_set readSetCopy = readSet;

    // Block for 5 seconds or until data is available to be read from SOL or user. Not currently checking max(solfd, commandfd) because commandfd is 0
    if(pselect((solfd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL) > 0) {
      
      // Check if SOL is the file descriptor that is available to be read
      if(FD_ISSET(solfd, &readSetCopy)) {
	// read one byte
	if(read(solfd, &readByte, sizeof(readByte)) < 0) {
	  BUException::IO_ERROR e;
	  //e.Append("read error: error reading from SOL\n");
	  e.Append(strerror(errno));
	  throw e;
	}
	printf("%c", readByte);
	// The last line does not have a newline, fflush is necessary for that last line
	fflush(stdout);
      }

      // Check if user inputted command
      if(FD_ISSET(STDIN, &readSetCopy)) {
	// read in one byte of user command and write it out to SOL
	if(read(STDIN, &writeByte, sizeof(writeByte)) < 0) {
	  BUException::IO_ERROR e;
	  //	  e.Append("read error: error reading command from stdin\n");
	  e.Append(strerror(errno));
	  throw e;
	}
	// 29 is group separator
	if(29 == (int)writeByte) {
	  printf("\n");
	  interactiveLoop = false;
	  continue;
	}
	if(write(solfd, &writeByte, sizeof(writeByte)) < 0) {
	  BUException::IO_ERROR e;
	  //	  e.Append("write error: error writing to SOL\n");
	  e.Append(strerror(errno));
	  throw e;
	}
      }
    }
  }

  // Restore old action of pressing Ctrl-C before returning (which is to kill the program)
  sigaction(SIGINT, &oldsa, NULL);
  
  // Leave curses mode
  endwin();

  return;
}

void IPMISOL::shutdown() {
  // Maybe we should catch something here. Not sure what destroy and teardown throw though.
  // Close file descriptor
  // Checkkk I think the following line would work the same
  //if(3 = ipmiconsole_ctx_status(ipmiContext) {
  if(-1 != solfd) {
    ipmiconsole_ctx_destroy(ipmiContext);
    solfd = -1;
    printf("Closing SOL ...\n");
    // Non zero parameter will cause SOL sessions to be torn down cleanly
    // Will block until all active ipmi sessions cleanly closed or timed out
    ipmiconsole_engine_teardown(1);
    printf("SOL closed\n");
   }
}
