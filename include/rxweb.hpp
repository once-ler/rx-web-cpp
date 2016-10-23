#pragma once

decltype(auto) RxEventLoop = rxcpp::observe_on_event_loop();
decltype(auto) RxNewThread = rxcpp::observe_on_new_thread();

using namespace std;

namespace rxweb {
  
  static string version = "0.1.0";

  template<typename T>
  struct task {
    using SocketType = SimpleWeb::ServerBase<T>;

    task(
      shared_ptr<typename SocketType::Request> req,
      shared_ptr<typename SocketType::Response> resp
    ) : request(req), response(resp) {
      ss = make_shared<stringstream>();
    }
    shared_ptr<typename SocketType::Response> response;
    shared_ptr<typename SocketType::Request> request;
    shared_ptr<std::stringstream> ss;
  };

  template<typename T>
  struct middleware {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using FilterFunc = std::function<bool(RxWebTask&)>;
    using MapFunc = std::function<RxWebTask&(RxWebTask&)>;

    FilterFunc filterFunc;
    MapFunc mapFunc;
    
    middleware(
      FilterFunc _filterFunc,
      MapFunc _mapFunc
    ) : filterFunc(_filterFunc), mapFunc(_mapFunc) {}
  };
  
}
