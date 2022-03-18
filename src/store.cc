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


// Let's implement a tiny state machine with the following states.
enum CallStatus { CREATE, PROCESS, FINISH };

// forward declarataion
class CallData;
void SubmitBidRequestsToAllVendors(std::string productName, CallData *clientRequestCallData);


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
        if (status_ == CREATE)
				{
							std::cout << "Proceed CREATE!\n";
							std::cout.flush();
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

						std::cout << "Proceed PROCESS!\n";
						std::cout.flush();

						SubmitBidRequestsToAllVendors(request_.product_name(), this);//&status_, &responder_, &reply_);

						// And we are done! Let the gRPC runtime know we've finished, using the
						// memory address of this instance as the uniquely identifying tag for
						// the event.

						//status_ = FINISH;
						//responder_.Finish(reply_, grpc::Status::OK, this);
        }
        else
        {
					std::cout << "Proceed FINISH!\n";
					std::cout.flush();

			      GPR_ASSERT(status_ == FINISH);
			      // Once in the FINISH state, deallocate ourselves (CallData).
			      delete this;
        }
    }

		void Finish()
		{
				status_ = FINISH;
				responder_.Finish(reply_, grpc::Status::OK, this);

				std::cout << "Got response! (" << reply_. << ", " << std::to_string(i) << ")\n";
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


    CallStatus status_;  // The current serving state.
};



void SubmitBidRequestsToAllVendors(std::string productName, CallData *clientRequestCallData)// CallStatus *status_, grpc::ServerAsyncResponseWriter<store::ProductReply> *responder_, store::ProductReply reply_)
{
		// The actual processing.
		// field this to a thread

		int numVendors = vendorIPaddresses.size();

		std::shared_ptr<grpc::Channel>         channel[numVendors];

		grpc::CompletionQueue                  completionQueue[numVendors];
		grpc::ClientContext 					         context[numVendors];
		vendor::BidQuery 							         bidQueryRequest[numVendors];
		std::unique_ptr<vendor::Vendor::Stub>  bidStub[numVendors];
		std::unique_ptr<grpc::ClientAsyncResponseReader<vendor::BidReply>>  bidResponseReader[numVendors];

		grpc::Status                           status[numVendors];
		vendor::BidReply                       bidReply[numVendors];


		for (int i = 0; i < numVendors; i++)
		{
				// establish a channel to make the request to the vendor
				channel[i] = grpc::CreateChannel(
						vendorIPaddresses[i],
						grpc::InsecureChannelCredentials()
				);

				// write the name to the request
				bidQueryRequest[i].set_product_name(productName);

				// create a stub using the channel
				bidStub[i] = vendor::Vendor::NewStub(channel[i]);

				// create an asynchronous response reader using the stub, request, and completion queue
				//   - this will watch for the response in the background
				//   - then read it into a BidReply
				//   - then let us know when the BidReply is available for reading via the completionQueue
				bidResponseReader[i] =
						std::unique_ptr<grpc::ClientAsyncResponseReader<vendor::BidReply>>
								(bidStub[i]->AsyncgetProductBid(&context[i], bidQueryRequest[i], &completionQueue[i]));

				// send the request (via the stub via the channel) and wait for the
				// response reader to put the reponse and status into bidReply / status
				bidResponseReader[i]->Finish(&bidReply[i], &status[i], (void*)(long long int) i);

				std::cout << "Sent request to " << vendorIPaddresses[i] << "\n";
		}

		// now we wait to get all the responses
		for (int i = 0; i < vendorIPaddresses.size(); i++)
		{
				void* got_tag;
				bool ok = false;

				completionQueue[i].Next(&got_tag, &ok);

				//std::cout << "Got response - ok is: " << std::to_string(ok) << "\n";

				if (ok && got_tag == (void*)(long long int) i) {
					std::cout << "Need to push these replies onto the big reply we send back!\n";
				    //std::cout << "Got response! (" << productName << ", " << std::to_string(i) << ")\n";
				}
		}

		clientRequestCallData->Finish();
		//clientRequestCallData->status_ = FINISH;
		//clientRequestCallData->responder_.Finish(clientRequestCallData->reply_, grpc::Status::OK, clientRequestCallData);
}








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
		std::string line;

		while(getline(vendorFile, line))
		{
			  vendorIPaddresses.push_back(line);
		}

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
