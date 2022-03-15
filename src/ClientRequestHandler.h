#ifndef CLIENT_REQUEST_HANDLER_H
#define CLIENT_REQUEST_HANDLER_H

#include <memory>

#include "Args.h"

#include "store.grpc.pb.h"

#include <grpcpp/grpcpp.h>

/*
// grpc namespace imports
using grpc::ServerCompletionQueue;
using grpc::Server;

// Store namespace imports
using store::Store;
*/

class ClientRequestHandler final {

private:


public:
		ClientRequestHandler();
		~ClientRequestHandler();

    int Initialize(Args args);

};

#endif
