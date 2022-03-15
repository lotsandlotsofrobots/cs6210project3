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
using grpc::InsecureServerCredentials;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;

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

		std::unique_ptr<grpc::ServerCompletionQueue>  completionQueue;
		store::Store::AsyncService     					      asyncService;
		std::unique_ptr<grpc::Server>                 server;

		ServerBuilder builder;
		builder.AddListeningPort(args.clientIPPort, InsecureServerCredentials());
		builder.RegisterService(&asyncService);

		completionQueue = builder.AddCompletionQueue();
		server = builder.BuildAndStart();

		ServerContext context;
		store::ProductQuery productQuery;

		grpc::ServerAsyncResponseWriter<store::ProductReply> response(&context);

		// this call does NOT block but it sends something out there and moves on
		// you'll get a response later,
		asyncService.RequestgetProducts(&context, &productQuery, &response, &(*completionQueue), &(*completionQueue), (void *) 1);

		void * tag;
		bool ok;

		completionQueue->Next(&tag, &ok);
		GPR_ASSERT(ok);

		std::cout << "got something!\n";
		std::cout.flush();



		server->Shutdown();
	  // Always shutdown the completion queue after the server.
	  completionQueue->Shutdown();


		std::cout << "I 'm not ready yet!" << std::endl;
		std::cout.flush();
		return EXIT_SUCCESS;
}
