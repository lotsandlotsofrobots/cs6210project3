#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <vector>

#include "ClientRequestHandler.h"
#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using std::cout;

using store::Store;
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

extern std::vector<std::string> vendorIPaddresses;

// Let's implement a tiny state machine with the following states.
enum CallStatus { CREATE, PROCESS, FINISH };

// forward declarataion
class CallData;
void SubmitBidRequestsToAllVendors(CallData *clientRequestCallData);
void EnqueWithThreadPool(CallData * clientRequestCallData);

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
						EnqueWithThreadPool(this);
        }
        else
        {
			      GPR_ASSERT(status_ == FINISH);
			      // Once in the FINISH state, deallocate ourselves (CallData).
			      delete this;
        }
    }

		void Finish()
		{
			  cout << "Got response! (" << request_.product_name() << ")\n";
				status_ = FINISH;
				responder_.Finish(reply_, grpc::Status::OK, this);

		}

		void AddProductInfo(vendor::BidReply * bidReply)
		{
			  store::ProductInfo * info = reply_.add_products();
				info->set_price(bidReply->price());
				info->set_vendor_id(bidReply->vendor_id());
		}

		std::string GetProductName()
		{
			  return request_.product_name();
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


class ThreadManager
{
public:
		ThreadManager(int i);
		void Start();
		void AssignCallData(CallData * callData);
		void SubmitBidRequestsToAllVendors();

		std::thread  t;
		int          threadID;
		CallData *   clientRequestCallData;
		bool         workAssigned;
};



class threadpool
{

public:

		threadpool() {}
		~threadpool() {}

	  threadpool(threadpool const&) = delete;
	  void operator=(threadpool const&) = delete;

		void InitThreads(int numberOfThreads)
		{
			  for (int i = 0; i < numberOfThreads; i++)
				{
					  ThreadManager * threadManager = new ThreadManager(i);
						availableThreads.push(threadManager);
				}
		}


		void AssignWork(CallData * callData)
		{
				threadsMutex.lock();
				requestsMutex.lock();

			  if (availableThreads.size() == 0)
				{

						std::cout << "No available threads, pushing onto request queue\n";

					  requests.push(callData);
						requestsMutex.unlock();
						threadsMutex.unlock();

						return;
				}

				ThreadManager * threadManager = availableThreads.front();
				availableThreads.pop();

				threadManager->AssignCallData(callData);

				requestsMutex.unlock();
				threadsMutex.unlock();
		}

		void EnqueThreadManager(ThreadManager * tm)
		{
				threadsMutex.lock();
				availableThreads.push(tm);
				threadsMutex.unlock();
		}

		static threadpool * Get()
    {
        static threadpool  thePool;
        return &thePool;
    }

		std::queue<ThreadManager*> availableThreads;
		std::queue<CallData*> requests;

		std::mutex threadsMutex;
		std::mutex requestsMutex;
};


void EnqueWithThreadPool(CallData * clientRequestCallData)
{
		threadpool::Get()->AssignWork(clientRequestCallData);
}



void Monitor(ThreadManager * t)
{
		while(true)
		{
				while(t->workAssigned == false)
				{
					std::this_thread::sleep_for (std::chrono::milliseconds(10));
					continue;
				}

				// we got some work!
				t->SubmitBidRequestsToAllVendors();
		}
}



ThreadManager::ThreadManager(int i)
{
	 workAssigned = false;
	 clientRequestCallData = NULL;
	 threadID = i;

	 t = std::thread(Monitor, this);
}

void ThreadManager::AssignCallData(CallData * callData)
{
		this->clientRequestCallData = callData;
		workAssigned = true;
}


void ThreadManager::SubmitBidRequestsToAllVendors()
{
		std::string productName = clientRequestCallData->GetProductName();
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
		}

		// now we wait to get all the responses
		for (int i = 0; i < vendorIPaddresses.size(); i++)
		{
				void* got_tag;
				bool ok = false;

				completionQueue[i].Next(&got_tag, &ok);

				if (ok && got_tag == (void*)(long long int) i) {
						clientRequestCallData->AddProductInfo(&bidReply[i]);
				}
		}

		clientRequestCallData->Finish();

		clientRequestCallData = NULL;
		workAssigned = false;
		threadpool::Get()->EnqueThreadManager(this);
}
