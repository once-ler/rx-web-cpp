#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <rxcpp/rx.hpp>
#include "server_ws.hpp"
#include "rxweb.hpp"
#include "subject.hpp"
#include "observer.hpp"
#include "subscriber.hpp"

namespace rxweb {
  // T: SimpleWeb::WS or SimpleWeb::WSS
  template<typename T>
  struct Endpoint {
    using WSAction = std::function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Message>)>;

    string expression;
    WSAction action;
    Endpoint(string expression_, WSAction action_) : expression(expression_), action(action_) {}
  };

  template<typename T>
  class wsserver {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using RxWebObserver = rxweb::observer<T>;
    using RxWebSubscriber = rxweb::subscriber<T>;
    using RxWebMiddleware = rxweb::middleware<T>;
    using WSServer = SimpleWeb::SocketServer<T>;
    // using decltype(auto) MessageHandler = std::function<void(shared_ptr<WSServer::Connection> connection, shared_ptr<WSServer::Message> message)>);

  public:
    // User provides custom Middleware.
    vector<RxWebMiddleware> middlewares;

    // Middleware that will handle the HTTP/S response. 
    RxWebMiddleware onNext;

    // User Defined Endpoints.
    vector<Endpoint<T>> endpoints;

    explicit wsserver(int _port, int _threads = 1) : port(_port), threads(_threads) {
      _server = make_shared<WSServer>();
      _server->config.port = port;
      _server->config.thread_pool_size = threads;
    }

  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType;
    shared_ptr<WSServer> _server;
    rxweb::subject<T> sub;
  };
}
