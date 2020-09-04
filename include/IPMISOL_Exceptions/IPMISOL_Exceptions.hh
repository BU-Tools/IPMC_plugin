#ifndef __EXCEPTIONHANDLER_HPP__
#define __EXCEPTIONHANDLER_HPP__

#include <BUException/ExceptionBase.hh>

namespace BUException {
  ExceptionClassGenerator(BAD_COMMAND_ERROR,"Bad command.\n")
  ExceptionClassGenerator(WRONG_ARGS_ERROR,"Wrong number of arguments.\n")
  ExceptionClassGenerator(BAD_ARGS_ERROR,"Bad arguments.\n")

  ExceptionClassGenerator(CONNECT_ERROR,"Error establishing connection.\n")
  ExceptionClassGenerator(IO_ERROR,"Error reading/writing from/to socket.\n")
}
  
#endif
