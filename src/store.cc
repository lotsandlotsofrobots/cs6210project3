// References:
//   - Async server:   https://grpc.io/docs/languages/cpp/async/
//   - ServerBuilder:  https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_builder.html
//   -

// local includes
#include "Args.h"
#include "ClientRequestHandler.h"
#include "store.grpc.pb.h"
#include "threadpool.h"

// grpc includes
#include <grpcpp/grpcpp.h>

// std includes
#include <fstream>
#include <iostream>
#include <memory>

// std namespace imports
using std::cout;
using std::stoi;
using std::string;
using std::unique_ptr;

// grpc namespace imports
using grpc::ServerCompletionQueue;
using grpc::Server;

// Store namespace imports
using store::Store;

/*
class ClientRequestHandler final {

private:
		unique_ptr<ServerCompletionQueue>  completionQueue;
		Store::AsyncService     					 asyncService;
		unique_ptr<Server>                 server;

public:
		ClientRequestHandler()
		{

		}
};
*/


int main(int argc, char** argv)
{
		if (argc != 4)
		{
			  std::cout << "Usage:  ./store [VENDOR_ADDRESS_FILE] [CLIENT_IP:CLIENT_PORT] [NUM_THREADS]\n";
				return 1;
		}

		Args args;
		ParseArgs(argv, args);

		ClientRequestHandler requestHandler;



		std::cout << "I 'm not ready yet!" << std::endl;
		return EXIT_SUCCESS;
}
