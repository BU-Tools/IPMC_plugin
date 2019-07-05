#ifndef __IPMISOL_DEVICE_HPP__
#define __IPMISOL_DEVICE_HPP__

//For tool device base class
#include <BUTool/CommandList.hh>

#include <BUTool/DeviceFactory.hh>

#include <string>
#include <vector>

#include <uhal/uhal.hpp>

#include <iostream>
#include <fstream>

#include <IPMISOL_Class/IPMISOL_Class.hh>

namespace BUTool{
  
  class IPMISOLDevice: public CommandList<IPMISOLDevice> {
  public:
    IPMISOLDevice(std::vector<std::string> arg); 
    ~IPMISOLDevice();

  private:
    IPMISOL_Class * myIPMISOL = NULL;
    
    //Here is where you update the map between string and function
    void LoadCommandList();

    //Add new command functions here
 
    CommandReturn::status Connect(std::vector<std::string>,std::vector<uint64_t>);
    CommandReturn::status Disconnect(std::vector<std::string>,std::vector<uint64_t>);
    CommandReturn::status talkToZynq(std::vector<std::string>,std::vector<uint64_t>);
 
    //Add new command (sub command) auto-complete files here
    std::string autoComplete_Help(std::vector<std::string> const &,std::string const &,int);

  };
  RegisterDevice(IPMISOLDevice,
		 "IPMISOL",
		 "",
		 "s",
		 "IPMISOL",
		 ""
		 ); //Register IPMISOLDevice with the DeviceFactory  
}

#endif
