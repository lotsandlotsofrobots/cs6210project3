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

std::vector<std::string> vendorIPaddresses;


class CallData {
public:
		// Take in the "service" instance (in this case representing an asynchronous
		// server) and the completion queue "cq" used for asynchronous communication
	  // with the gRPC runtime.
		CallData(Store::AsyncService* service, ServerCompletionQueue* cq)
				 : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
			 	 // Invoke the serving logic right away.
			 	Proceed();
		}

		void Proceed()
		{
				std::cout << "Proceed !\n";
				std::cout.flush();

        if (status_ == CREATE)
				{
			 			// Make this instance progress to the PROCESS state.
			 			status_ = PROCESS;

					 // As part of the initial CREATE state, we *request* that the system
					 // start processing SayHello requests. In this request, "this" acts are
					 // the tag uniquely identifying the request (so that different CallData
					 // instances can serve different requests concurrently), in this case
					 // the memory address of this CallData instance.
					 service_->RequestgetProducts(&ctx_, &request_, &responder_, cq_, cq_,
																		 this);
        }
        else if (status_ == PROCESS)
				{
						// Spawn a new CallData instance to serve new clients while we process
						// the one for this CallData. The instance will deallocate itself as
						// part of its FINISH state.
						new CallData(service_, cq_);

						// The actual processing.

						// field this to a thread
						{


								for (int i = 0; i < vendorIPaddresses.size(); i++)
								{
										/*MathTestClient client(
											grpc::CreateChannel(
												address,
												grpc::InsecureChannelCredentials()
											)
										);*/

										std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
												vendorIPaddresses[i],
												grpc::InsecureChannelCredentials()
										);

										grpc::CompletionQueue completionQueue;
										grpc::ClientContext context;

										vendor::BidQuery request;
										request.set_product_name(request_.product_name());

										std::unique_ptr<vendor::Vendor::Stub> stub_ = vendor::Vendor::NewStub(channel);
										std::unique_ptr<grpc::ClientAsyncResponseReader<vendor::BidReply>> rpc(stub_->AsyncgetProductBid(&context, request, &completionQueue));

										grpc::Status status;
										vendor::BidReply bidReply;
										rpc->Finish(&bidReply, &status, (void*) this);

										//VendorClient vendor_client(grpc::CreateChannel(server_addr, grpc::InsecureChannelCredentials()));
										//store_client.callGetProducts(product_name, pq_result);

								}

						}



						// And we are done! Let the gRPC runtime know we've finished, using the
						// memory address of this instance as the uniquely identifying tag for
						// the event.

						status_ = FINISH;
						responder_.Finish(reply_, grpc::Status::OK, this);
        }
        else
        {
			      GPR_ASSERT(status_ == FINISH);
			      // Once in the FINISH state, deallocate ourselves (CallData).
			      delete this;
        }
    }

private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Store::AsyncService* service_;

    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;

    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    store::ProductQuery request_;

    // What we send back to the client.
    store::ProductReply reply_;

    // The means to get back to the client.
    grpc::ServerAsyncResponseWriter<store::ProductReply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
};



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

		//ServerContext context;
		//store::ProductQuery productQuery;
		std::ifstream vendorFile(args.vendorAddressFile);

/*
		grpc::ServerAsyncResponseWriter<store::ProductReply> response(&context);

		// this call does NOT block but it sends something out there and moves on
		// you'll get a response later, it has to be snagged by Next()
		asyncService.RequestgetProducts(&context, &productQuery, &response, &(*completionQueue), &(*completionQueue), (void *) 1);

		void * tag;
		bool ok;

		completionQueue->Next(&tag, &ok);
		GPR_ASSERT(ok);

		std::cout << "got something!\n";
		std::cout.flush();
*/

		std::cout << "Entering the calldata loop!\n";
		std::cout.flush();

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
	  // Always shutdown the completion queue after the server.
	  completionQueue->Shutdown();


		std::cout << "I 'm not ready yet!" << std::endl;
		std::cout.flush();
		return EXIT_SUCCESS;
}
