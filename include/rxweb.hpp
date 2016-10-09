#pragma once

#include <memory>
#include <vector>
#include "server_http.hpp"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using Request = std::shared_ptr<HttpServer::Request>;
using Response = std::shared_ptr<HttpServer::Response>;

namespace rxweb {  
  struct task {
    task(Request req, Response resp) : request(req), response(resp) {}
    Response response;
    Request request;
    std::vector<int> traceIds;
  };  
}
