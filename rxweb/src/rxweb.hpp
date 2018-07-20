#pragma once

#include <iostream>
#include <rxcpp/rx.hpp>
#include "json.hpp"
#include "server_http.hpp"
#include "server_ws.hpp"

decltype(auto) RxEventLoop = rxcpp::observe_on_event_loop();
decltype(auto) RxNewThread = rxcpp::observe_on_new_thread();

using namespace std;
using json = nlohmann::json;

namespace rxweb {
  
  static string version = "0.7.0";

  // Just a utility.
  std::hash<std::thread::id> hasher;

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
    shared_ptr<typename SocketType::Request> request;
    shared_ptr<typename SocketType::Response> response;
    shared_ptr<std::stringstream> ss;
    string type;
    shared_ptr<json> data;
  };

  template<typename T>
  struct wstask {
    using WebSocketType = SimpleWeb::SocketServerBase<T>;

    wstask() {
      ss = make_shared<stringstream>();
      data = make_shared<json>();
    }
    wstask(
      shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection> conn,
      shared_ptr<typename SimpleWeb::SocketServerBase<T>::Message> msg = nullptr
    ) : connection(conn), message(msg) {
      ss = make_shared<stringstream>();
      data = make_shared<json>();
    }
    shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection> connection;
    shared_ptr<typename SimpleWeb::SocketServerBase<T>::Message> message;
    string path;
    shared_ptr<std::stringstream> ss;
    string type;
    shared_ptr<json> data;    
  };
  
  template<typename T>
  struct middleware {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using FilterFunc = std::function<bool(const RxWebTask&)>;
    using SubscribeFunc = std::function<void(const RxWebTask&)>;
  
    FilterFunc filterFunc;
    SubscribeFunc subscribeFunc;

    middleware() = default;

    middleware(
      FilterFunc _filterFunc,
      SubscribeFunc _subscribeFunc
    ) : filterFunc(_filterFunc), subscribeFunc(_subscribeFunc) {}
    
    middleware(FilterFunc _filterFunc) : filterFunc(_filterFunc) {}
  };

  template<typename T>
  struct wsmiddleware {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using RxWsTask = rxweb::wstask<T>;
    using FilterFunc = std::function<bool(const RxWsTask&)>;
    using SubscribeFunc = std::function<void(const RxWsTask&)>;

    FilterFunc filterFunc;
    SubscribeFunc subscribeFunc;

    wsmiddleware() = default;

    wsmiddleware(
      FilterFunc _filterFunc,
      SubscribeFunc _subscribeFunc
    ) : filterFunc(_filterFunc), subscribeFunc(_subscribeFunc) {
    }

    wsmiddleware(FilterFunc _filterFunc) : filterFunc(_filterFunc) {}
  };
  
  // Default Exception Handler Handler
  void handleEptr(std::exception_ptr eptr) {
    try {
      if (eptr) {
        std::rethrow_exception(eptr);
      }
    } catch (const std::exception& e) {
      std::cout << "Caught exception \"" << e.what() << "\"\n";
    }
  }
}
