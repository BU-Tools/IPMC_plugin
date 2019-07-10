#include "IPMISOL_device/IPMISOL_device.hh"
//#include <BUException/ExceptionBase.hh>
#include <IPMISOL_Exceptions/IPMISOL_Exceptions.hh>
#include <boost/regex.hpp>

//For networking constants and structs                                                                          
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> //for inet_ntoa        

#include <ctype.h> //for isdigit

using namespace BUTool;

IPMISOLDevice::IPMISOLDevice(std::vector<std::string> arg)
  : CommandList<IPMISOLDevice>("IPMISOL"),
    myIPMISOL(NULL) {

  // Constructor currently takes exactly one argument
  if(1 != arg.size()) {
    BUException::WRONG_ARGS_ERROR e;
    e.Append("IPMISOLDevice takes exactly one argument: The IP address\n");
    throw e;
  }
  
  ip = arg[0];

  //setup commands
  LoadCommandList();
}

IPMISOLDevice::~IPMISOLDevice(){
  if(myIPMISOL) {
    delete myIPMISOL;
  } 
}

  
void IPMISOLDevice::LoadCommandList(){
    // general commands (Launcher_commands)

  AddCommand("connect",&IPMISOLDevice::Connect,
	     "establish ipmi SOL connection\n" \
	     "Usage: \n"                       \
	     "  connect\n");
  AddCommandAlias("c","connect");

  AddCommand("disconnect",&IPMISOLDevice::Disconnect,
	     "cleanly disconnect ipmi SOL connection\n" \
	     "Usage: \n"                                \
	     "  disconnect\n");
  AddCommandAlias("d","disconnect");

  AddCommand("SOLConsole",&IPMISOLDevice::SOLConsole,
	     "interact through SOL\n" \
	     "Usage: \n"                \
	     "  SOLConsole\n");
  AddCommandAlias("sc","SOLConsole");
}

CommandReturn::status IPMISOLDevice::Connect(std::vector<std::string>,std::vector<uint64_t>) {

  if(myIPMISOL) {
    printf("ipmi SOL connection already established\n");
    return CommandReturn::OK;
  }

  //  std::string ip = "192.168.20.56";

  try {
    myIPMISOL = new IPMISOL_Class(ip);
  } catch(BUException::CONNECT_ERROR & e) {
    throw e;
  }

  printf("ipmi SOL connection established at address: %s\n", ip.c_str());
  return CommandReturn::OK;
}

CommandReturn::status IPMISOLDevice::Disconnect(std::vector<std::string>,std::vector<uint64_t>) {

  if(myIPMISOL) {
    delete myIPMISOL;
    myIPMISOL = NULL;
  } else {
    printf("No established ipmi SOL connection to disconnect from\n");
  }
  
  return CommandReturn::OK;
}

CommandReturn::status IPMISOLDevice::SOLConsole(std::vector<std::string>,std::vector<uint64_t>) {

  if(!myIPMISOL) {
    printf("ipmi SOL connection not yet established\n");
    return CommandReturn::OK;
  }

  myIPMISOL->SOLConsole();

  return CommandReturn::OK;
}
