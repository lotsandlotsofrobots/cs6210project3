// References:
//   - Async server:   https://grpc.io/docs/languages/cpp/async/
//   - ServerBuilder:  https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_builder.html
//   -

// local includes
#include "Args.h"
#include "ClientRequestHandler.h"
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"
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
using std::to_string;
using std::unique_ptr;

// grpc namespace imports
using grpc::InsecureServerCredentials;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;

// Store namespace imports
using store::Store;

std::vector<std::string> vendorIPaddresses;

int main(int argc, char** argv)
{
		if (argc != 4)
		{
			  cout << "Usage:  ./store [VENDOR_ADDRESS_FILE] [CLIENT_IP:CLIENT_PORT] [NUM_THREADS]\n";
				return 1;
		}

		Args args;
		ParseArgs(argv, args);

		std::unique_ptr<grpc::ServerCompletionQueue>  completionQueue;
		store::Store::AsyncService     					      asyncService;
		std::unique_ptr<grpc::Server>                 server;

		ServerBuilder builder;
		builder.AddListeningPort(args.clientIPPort, InsecureServerCredentials());
		builder.RegisterService(&asyncService);

		completionQueue = builder.AddCompletionQueue();
		server = builder.BuildAndStart();

		std::ifstream vendorFile(args.vendorAddressFile);
		std::string line;

		while(getline(vendorFile, line))
		{
			  vendorIPaddresses.push_back(line);
		}

		threadpool::Get()->InitThreads(args.numberOfThreads);

		cout << "Entering the calldata loop!\n";
		cout.flush();

		new CallData(&asyncService, completionQueue.get());

		void *tag;
		bool ok;

		while (true)
		{
		    // Block waiting to read the next event from the completion queue. The
		    // event is uniquely identified by its tag, which in this case is the
		    // memory address of a CallData instance.
		    completionQueue->Next(&tag, &ok);
		    GPR_ASSERT(ok);
		    static_cast<CallData*>(tag)->Proceed();
	  }

		server->Shutdown();
	  completionQueue->Shutdown();

		cout << "I 'm not ready yet!" << std::endl;
		cout.flush();
		return EXIT_SUCCESS;
}
