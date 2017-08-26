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

using json = nlohmann::json;

namespace rxweb {
  // T: SimpleWeb::WS or SimpleWeb::WSS
  template<typename T>
  struct WsRoute {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using WsAction = std::function<void(shared_ptr<typename SocketType::Connection>, shared_ptr<typename SocketType::Message>)>;

    string expression;
    WsAction action;
    WsRoute(string expression_, WsAction action_) : expression(expression_), action(action_) {}
  };

  template<typename T>
  class wsserver {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using RxWsTask = rxweb::wstask<T>;
    using RxWsObserver = rxweb::wsobserver<T>;
    using RxWsSubscriber = rxweb::wssubscriber<T>;
    using RxWsMiddleware = rxweb::wsmiddleware<T>;
    using WsServer = SimpleWeb::SocketServer<T>;
    using MessageHandler = function<void(shared_ptr<typename SocketType::Connection>, shared_ptr<typename SocketType::Message>)>;
    using ErrorHandler = function<void(shared_ptr<typename SocketType::Connection>, const SimpleWeb::error_code&)>;
    using OpenHandler = function<void(shared_ptr<typename SocketType::Connection>)>;
    using CloseHandler = function<void(shared_ptr<typename SocketType::Connection>, int, const string&)>;
    using WsAction = std::function<void(shared_ptr<typename SocketType::Connection>, shared_ptr<typename SocketType::Message>)>;

    MessageHandler handleMesssge = [this](shared_ptr<SocketType::Connection> connection, shared_ptr<SocketType::Message> message) {
      cout << connection->path << endl;
      cout << connection->query_string << endl;

      auto sub = getSubject();
      auto t = RxWsTask{ connection, message };
      auto msg = message->string();

      try {
        auto j = json::parse(msg);
        t.data = make_shared<json>(j);
      } catch (...) {
        t.ss = make_shared<stringstream>(msg);
      }

      t.type = "ON_MESSAGE";
      sub.subscriber().on_next(t);
    };

    ErrorHandler handleError = [this](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
      auto sub = getSubject();
      auto t = RxWsTask{ connection, nullptr };
      json j = {
        { "errorCode", {
            "name", ec.category().name(),
            "value", ec.value()
          }
        }
      };
      t.data = make_shared<json>(j);
      t.type = "ON_ERROR";
      sub.subscriber().on_next(t);
    };

    OpenHandler handleOpen = [this](shared_ptr<WsServer::Connection> connection) {
      auto sub = getSubject();
      auto t = RxWsTask{ connection, nullptr };
      t.type = "ON_OPEN";
      sub.subscriber().on_next(t);
    };

    CloseHandler handleClose = [this](shared_ptr<WsServer::Connection> connection, int status, const string& reason) {
      auto sub = getSubject();
      auto t = RxWsTask{ connection, nullptr };
      json j = {
        { "status", status }
      };
      t.data = make_shared<json>(j);
      t.type = "ON_CLOSE";
      sub.subscriber().on_next(t);
    };


  public:
    // User provides custom Middleware.
    vector<RxWsMiddleware> middlewares;

    // Middleware that will handle the HTTP/S response. 
    // RxWsMiddleware onNext;

    // User Defined routes.
    vector<WsRoute<T>> routes;

    // Endpoints
    // std::map<SocketType::Endpoint, WsAction> endpoints;

    explicit wsserver(int _port, int _threads = 1) : port(_port), threads(_threads) {
      _server = make_shared<WsServer>();
      _server->config.port = port;
      _server->config.thread_pool_size = threads;
    }

    explicit wsserver(
      int _port,
      int _threads,
      const string _certFile,
      const string _privateKeyFile
    ) : port(_port), threads(_threads), certFile(_certFile), privateKeyFile(_privateKeyFile) {
      // Ignore certs if HTTP is specified.
      if (std::is_same<SocketType, SimpleWeb::WS>) {
        _server = make_shared<WsServer>();        
      } else {
        _server = make_shared<WsServer>(certFile, privateKeyFile);
      }
      _server->config.port = port;
      _server->config.thread_pool_size = threads;
    }

    void applyRoutes() {
      std::for_each(routes.begin(), routes.end(), [&, this](const WsRoute<T>& r) {
        auto& endpoint = _server->endpoint[r.expression];
        endpoint.on_open = handleOpen;
        endpoint.on_message = handleMesssge;
        endpoint.on_error = handleError;
        endpoint.on_close = handleClose;

        // endpoints[endpoint] = r.action;
      });
    }

    rxweb::wssubject<T> getSubject() {
      return sub;
    }

    void start() {
      makeObserversAndSubscribeFromMiddlewares();

      // Apply user-defined routes
      applyRoutes();

      _server->start();
    }

  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType, endpoint;
    shared_ptr<WsServer> _server;
    rxweb::wssubject<T> sub;

    void makeObserversAndSubscribeFromMiddlewares() {
      // No subscription, observers does nothing.      
      // Create Observers that react to subscriber broadcast.
      std::for_each(middlewares.begin(), middlewares.end(), [&](auto& route) {
        RxWsObserver observer(sub.observable(), route.filterFunc);
        observer.subscribe(route.subscribeFunc);
      });
    }
  };
}
