#pragma once

#include <memory>
#include <vector>
#include <memory>
#include <rxcpp/rx.hpp>

using namespace std;

decltype(auto) RxEventLoop = rxcpp::observe_on_event_loop();

std::hash<std::thread::id> hasher;

namespace rxweb {
  
  template<typename T>
  struct task {
    task(
      shared_ptr<typename SimpleWeb::ServerBase<T>::Request> req, 
      shared_ptr<typename SimpleWeb::ServerBase<T>::Response> resp
    ) : request(req), response(resp) {}
    shared_ptr<typename SimpleWeb::ServerBase<T>::Response> response;
    shared_ptr<typename SimpleWeb::ServerBase<T>::Request> request;
    std::vector<int> traceIds;
  };

  template<typename T>
  struct Route {
    std::function<bool(rxweb::task<T>&)> filterFunc;
    std::function<rxweb::task&(rxweb::task<T>&)> mapFunc;
    Route(
      std::function<bool(rxweb::task<T>&)> _filterFunc, 
      std::function<rxweb::task&(rxweb::task<T>&)> _mapFunc
    ) : filterFunc(_filterFunc), mapFunc(_mapFunc) {}
  };
  
}
