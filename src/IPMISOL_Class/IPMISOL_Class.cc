#include <IPMISOL_Class/IPMISOL_Class.hh>
#include <string.h> // strlen
#include <unistd.h>
#include <IPMISOL_Exceptions/IPMISOL_Exceptions.hh>

// Constructor
IPMISOL_Class::IPMISOL_Class(std::string const & _ipmc_ip_addr) {
  Connect(_ipmc_ip_addr);
}

// Deconstructor
IPMISOL_Class::~IPMISOL_Class() {
  // shutdown will check if a proper connection was established and if so, terminate it
  shutdown();
}

// A do-it-all function to set up the SOL connection
void IPMISOL_Class::Connect(std::string const & _ip) {
  // Always initialize non-const file descriptors as invalid
  solfd = -1;
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

  // Unfortunately &SOL_PAYLOAD_NUM does not work
  int num = SOL_PAYLOAD_NUM;
  
  // Configure context
  ipmiconsole_ctx_set_config(ipmiContext, IPMICONSOLE_CTX_CONFIG_OPTION_SOL_PAYLOAD_INSTANCE, (void *)&num);

  // Submit context to engine
  // returns 0 on success. No callback function (2nd arg) or callback parameters (3rd arg)
  ipmiconsole_engine_submit(ipmiContext, NULL, NULL);

  // get file descriptor to talk to and read from
  solfd = ipmiconsole_ctx_fd(ipmiContext);

  // Put file descrptor in set of descriptors to read from
  FD_SET(solfd, &readSet);
  FD_SET(commandfd, &readSet);
  
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

void IPMISOL_Class::configUser(ipmiconsole_ipmi_config * const user) {
  user->username = (char *)SOLUSERNAME;
  user->password = (char *)SOLPASSWORD;
  user->k_g = (unsigned char *)K_G;
  user->k_g_len = strlen(K_G) + 1;
  user->privilege_level = IPMICONSOLE_PRIVILEGE_USER;
  user->cipher_suite_id = 0;
  user->workaround_flags = IPMICONSOLE_WORKAROUND_DEFAULT;
}

void IPMISOL_Class::configProtocol(ipmiconsole_protocol_config * const prot) {
  prot->session_timeout_len = -1;
  prot->retransmission_timeout_len = -1;
  prot->retransmission_backoff_count = -1;
  prot->keepalive_timeout_len = -1;
  prot->retransmission_keepalive_timeout_len = -1;
  prot->acceptable_packet_errors_count = -1;
  prot->maximum_retransmission_count = -1;
}

void IPMISOL_Class::configEngine(ipmiconsole_engine_config * const engine) {
  engine->engine_flags = IPMICONSOLE_ENGINE_DEFAULT;
  //engine->engine_flags = IPMICONSOLE_ENGINE_OUTPUT_ON_SOL_ESTABLISHED;
  engine->behavior_flags = IPMICONSOLE_BEHAVIOR_DEFAULT;
  engine->debug_flags = IPMICONSOLE_DEBUG_DEFAULT;
  // STDERR is very useful and very verbose
  //engine->debug_flags = IPMICONSOLE_DEBUG_STDERR;
}

bool interactiveLoop;

//void IPMISOL_Class::signal_handler(int const signum) {
/*
void signal_handler(int const signum) {
  if(SIGINT == signum) {
    interactiveLoop = false;
  }
  return;
}
*/

// The function where all the talking to and reading from zynq through SOL happens
void IPMISOL_Class::talkToZynq() {
  char readByte;
  char writeByte;

  // Instantiate sigaction struct member with signal handler function 
  //sa.sa_handler = signal_handler;
  // Catch SIGINT and pass to function signal_handler within sigaction struct sa
  //sigaction(SIGINT, &sa, &oldsa);
  interactiveLoop = true;

  while(interactiveLoop) {
    // Make a copy every time readSet is passed to pselect because pselect changes contents of fd_sets and we don't want readSet to change
    fd_set readSetCopy = readSet;

    int n;
    // Block for 5 seconds or until data is available to be read from zynq or user. Not currently checking max(solfd, commandfd) because commandfd is 0
    if((n = pselect((solfd + 1), &readSetCopy, NULL, NULL, &timeoutStruct, NULL)) > 0) {

      //printf("n inside is %d\n", n);
      
      // Check if zynq is the file descriptor that is available to be read
      if(FD_ISSET(solfd, &readSetCopy)) {
	// read one byte
	if(read(solfd, &readByte, sizeof(readByte)) < 0) {
	  BUException::IO_ERROR e;
	  e.Append("read error: error reading from zynq\n");
	  throw e;
	}
	if(0x19 != readByte) {printf("%c", readByte);}
      }

      // Check if user inputted command
      if(FD_ISSET(commandfd, &readSetCopy)) {
	// read in one byte of user command and write it out to zynq
	if(read(commandfd, &writeByte, sizeof(writeByte)) < 0) {
	  BUException::IO_ERROR e;
	  e.Append("read error: error reading command from stdin\n");
	  throw e;
	}
	// 29 is group separator
	if(29 == (int)writeByte) {
	  interactiveLoop = false;
	  continue;
	}
	if(write(solfd, &writeByte, sizeof(writeByte)) < 0) {
	  BUException::IO_ERROR e;
	  e.Append("write error: error writing to zynq\n");
	  throw e;
	}
      }
    }
    //printf("n is %d\n", n);
  }

  // Restore old action of pressing Ctrl-C before returning (which is to kill the program)
  //sigaction(SIGINT, &oldsa, NULL);
  
  return;
}

void IPMISOL_Class::shutdown() {
  // Close file descriptor
  // Checkkk I think the following line would work the same
  //if(3 = ipmiconsole_ctx_status(ipmiContext) {
  if(-1 != solfd) {
    ipmiconsole_ctx_destroy(ipmiContext);
    printf("SOL file descriptor succesfully closed\n");
    printf("Closing SOL session ...\n");
    // Non zero parameter will cause SOL sessions to be torn down cleanly
    // Will block until all active ipmi sessions cleanly closed or timed out
    ipmiconsole_engine_teardown(1);
    printf("SOL session successfully terminated\n");   
   }
}
