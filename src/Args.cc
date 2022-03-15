#include "Args.h"

int ParseArgs(char** argv, Args &a)
{
		a.vendorAddressFile = std::string(argv[1]);
    a.clientIPPort = std::string(argv[2]);

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
