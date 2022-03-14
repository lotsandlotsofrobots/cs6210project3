#include "Args.h"

int ParseArgs(char** argv, Args &a)
{
		a.vendorAddressFile = std::string(argv[1]);

		try
		{
			  a.clientIPPort = std::stoi(argv[2]);
		}
		catch(std::exception &e)
		{
			  std::cout << "Could not decode clientIPPort from " << std::string(argv[2]) << "\n";
				return -1;
		}

		try
		{
			  a.numberOfThreads = std::stoi(argv[3]);
		}
		catch(std::exception &e)
		{
			  std::cout << "Could not decode numberOfThreads from " << std::string(argv[3]) << "\n";
				return -1;
		}

		return 0;
}
