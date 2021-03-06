#ifndef __ARGS_H_
#define __ARGS_H_

#include <iostream>
#include <string>

typedef struct Args
{
		std::string vendorAddressFile;
		std::string clientIPPort;
		int 			  numberOfThreads;
} Args;

int ParseArgs(char** argv, Args &a);

#endif
