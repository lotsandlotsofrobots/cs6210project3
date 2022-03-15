#include <memory>
#include <stdlib.h>

#include <grpc++/grpc++.h>

#include "store.grpc.pb.h"

#include "product_queries_util.h"

using store::Store;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using store::ProductQuery;
using store::ProductReply;
using store::ProductInfo;

class StoreClient {

 public:
  StoreClient(std::shared_ptr<Channel>);
  bool getProducts(const ProductSpec&, ProductQueryResult&);

 private:
  std::unique_ptr<Store::Stub> stub_;
};


bool run_client(const std::string& server_addr, const std::string& product_name, ProductQueryResult& pq_result) {

  std::cout << "Creating store_client\n";
  std::cout.flush();

  StoreClient store_client(grpc::CreateChannel(server_addr, grpc::InsecureChannelCredentials()));

  std::cout << "returning store client\n";
  std::cout.flush();

  return store_client.getProducts(product_name, pq_result);
}


StoreClient::StoreClient(std::shared_ptr<Channel> channel)
  : stub_(Store::NewStub(channel))
  {}

bool StoreClient::getProducts(const ProductSpec& product_spec, ProductQueryResult& query_result) {
  ProductQuery query;
  query.set_product_name(product_spec.name_);

  ProductReply reply;
  ClientContext context;

  std::cout << "trying to get products\n";
  std::cout.flush();

  Status status = stub_->getProducts(&context, query, &reply);

  std::cout << "finished get products\n";
  std::cout.flush();

  if (!status.ok()) {
    std::cout << "An error occurred in getProducts: " << status.error_code() << ": " << status.error_message()
              << std::endl;
    std::cout.flush();
    
    return false;
  }

  std::cout << "status was okay?\n";
  std::cout.flush();

  for (const auto result : reply.products()) {
    ProductQueryResult::Bid bid;
    bid.price_ = result.price();
    bid.vendor_id_ = result.vendor_id();
    query_result.bids_.push_back(bid);
  }

  std::cout << "Returning true?\n";
  std::cout.flush();

  return true;
}
