#ifndef __IPMISOL_CLASS_HH__
#define __IPMISOL_CLASS_HH__

#include <ipmiconsole.h>
#include <string>

#define SOLUSERNAME "soluser"
#define SOLPASSWORD "solpasswd"
#define K_G "root"

// SOL payload instance number. Most systems support only a single instance. Some systems allow users to access multiple (meaning multiple users could then see the same serial session, for example)
#define SOL_PAYLOAD_NUM 1

class IPMISOL_Class {
public:
  IPMISOL_Class(std::string const & _ipmc_ip_addr);

  char readByte();
  //std::string readLine();
  char writeByte(char const data);
  char writeLine(std::string const & message);

  // Function where all the talking and reading through SOL happens
  void SOLConsole();
  
  ~IPMISOL_Class();
  
private:

  std::string ipmc_ip_addr;
  
  // Set of file descriptors to be read from. If our fd is 9, there are 10 elements, nine 0s in the beginning and then one 1
  fd_set readSet;
  // For pselect timeout
  struct timespec timeoutStruct;
  
  // specify sol user info
  struct ipmiconsole_ipmi_config ipmiUser;
  // specify protocol
  struct ipmiconsole_protocol_config ipmiProtocol;
  // specify engine confiurations
  struct ipmiconsole_engine_config ipmiEngine;

  // Declare a context (you get a file descriptor from the context)
  ipmiconsole_ctx_t ipmiContext;

  // File descriptor to read from SOL
  int solfd;
  // File descriptor to read user commands
  int const commandfd = 0;  // Will be 0 (aka stdin)

  // configure user info
  void configUser(ipmiconsole_ipmi_config * const user);
  // configure protocol specificatinoms
  void configProtocol(ipmiconsole_protocol_config * const prot);
  // configure engine
  void configEngine(ipmiconsole_engine_config * const eng);

  // Do it all function to set up sol connection
  void Connect(std::string const & _ip);

  // Clearly terminate SOL connection
  void shutdown();
};

#endif
