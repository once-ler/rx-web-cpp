#pragma once

#include <iostream>
#include <regex>
#include <vector>
#include <memory>
#include <rxcpp/rx.hpp>
#include "server_ws.hpp"
#include "rxweb.hpp"
#include "subject.hpp"
#include "observer.hpp"
#include "subscriber.hpp"

using namespace std;
using json = nlohmann::json;

namespace rxweb {
  // T: SimpleWeb::WS || SimpleWeb::WSS
  template<typename T>
  class WsRoute {
    template<typename T>friend class wsserver;

    using SocketType = SimpleWeb::SocketServerBase<T>;
    using WsAction = std::function<void(shared_ptr<typename SocketType::Connection>, shared_ptr<typename SocketType::Message>)>;
  public:
    WsRoute(string expression_, WsAction action_) : expression(expression_), action(action_) {}
    string expression;
    WsAction action;

  private:
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

      for (auto& r : routes) {
        regex path_rx(r.expression);
        bool m = regex_search(connection->path, path_rx);
        if (m) r.action(connection, message);
      }
    };

    ErrorHandler handleError = [this](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
      auto sub = getSubject();
      auto t = RxWsTask{ connection };
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
      auto t = RxWsTask{ connection };
      t.type = "ON_OPEN";
      sub.subscriber().on_next(t);
    };

    CloseHandler handleClose = [this](shared_ptr<WsServer::Connection> connection, int status, const string& reason) {
      auto sub = getSubject();
      auto t = RxWsTask{ connection };
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
      });
    }

    rxweb::wssubject<T> getSubject() {
      return sub;
    }
    
    void dispatch(const RxWsTask& t) {
      sub.subscriber().on_next(t);
    }
    
    void broadcast(const string message) {
      for (auto& r : routes) {
        auto& endpoint = _server->endpoint[r.expression];
        for (auto& e : endpoint.get_connections()) {
          auto send_stream = make_shared<SocketType::SendStream>();
          *send_stream << message;
          e->send(send_stream, [](const SimpleWeb::error_code &ec) {});
        }
      }
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
