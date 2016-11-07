#pragma once

#include <rxcpp/rx.hpp>
#include "json.hpp"

decltype(auto) RxEventLoop = rxcpp::observe_on_event_loop();
decltype(auto) RxNewThread = rxcpp::observe_on_new_thread();

using namespace std;
using json = nlohmann::json;

namespace rxweb {
  
  static string version = "0.3.0";

  template<typename T>
  struct task {
    using SocketType = SimpleWeb::ServerBase<T>;

    task() {
      ss = make_shared<stringstream>();
      data = make_shared<json>();
    }
    task(
      shared_ptr<typename SocketType::Request> req,
      shared_ptr<typename SocketType::Response> resp
    ) : request(req), response(resp) {
      ss = make_shared<stringstream>();
      data = make_shared<json>();
    }
    shared_ptr<typename SocketType::Response> response;
    shared_ptr<typename SocketType::Request> request;
    shared_ptr<std::stringstream> ss;
    shared_ptr<json> data;
  };

  template<typename T>
  struct middleware {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using FilterFunc = std::function<bool(RxWebTask&)>;
    using MapFunc = std::function<RxWebTask&(RxWebTask&)>;
    
    FilterFunc filterFunc;
    MapFunc mapFunc;
    
    middleware() = default;

    middleware(
      FilterFunc _filterFunc,
      MapFunc _mapFunc
    ) : filterFunc(_filterFunc), mapFunc(_mapFunc) {}
        
    middleware(MapFunc _mapFunc) : mapFunc(_mapFunc) {}
  };
  
}
