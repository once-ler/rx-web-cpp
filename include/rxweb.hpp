#pragma once

#include <memory>
#include <vector>
#include <memory>;
#include <rxcpp/rx.hpp>

using namespace std;

decltype(auto) RxEventLoop = rxcpp::observe_on_event_loop();

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using Request = std::shared_ptr<HttpServer::Request>;
using Response = std::shared_ptr<HttpServer::Response>;

std::hash<std::thread::id> hasher;

namespace rxweb {
  
  struct task {
    task(Request req, Response resp) : request(req), response(resp) {}
    Response response;
    Request request;
    std::vector<int> traceIds;
  };

  using FilterFunc = std::function<bool(rxweb::task&)>;
  using MapFunc = std::function<rxweb::task&(rxweb::task&)>;

  struct Route {
    FilterFunc filterFunc;
    MapFunc mapFunc;
    Route(FilterFunc _filterFunc, MapFunc _mapFunc) : filterFunc(_filterFunc), mapFunc(_mapFunc) {}
  };
  
}
