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
  struct Endpoint {
    using WsAction = std::function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Message>)>;

    string expression;
    WsAction action;
    Endpoint(string expression_, WsAction action_) : expression(expression_), action(action_) {}
  };

  template<typename T>
  class wsserver {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using RxWsTask = rxweb::wstask<T>;
    using RxWsObserver = rxweb::wsobserver<T>;
    using RxWsSubscriber = rxweb::wssubscriber<T>;
    using RxWsMiddleware = rxweb::wsmiddleware<T>;
    using WsServer = SimpleWeb::SocketServer<T>;
    using MessageHandler = function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection>, shared_ptr<typename SimpleWeb::SocketServerBase<T>::Message>)>;
    using ErrorHandler = function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection>, const SimpleWeb::error_code&)>;
    using OpenHandler = function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection>)>;
    using CloseHandler = function<void(shared_ptr<typename SimpleWeb::SocketServerBase<T>::Connection>, int, const string&)>;

    MessageHandler handleMesssge = [this](shared_ptr<SimpleWeb::SocketServerBase<T>::Connection> connection, shared_ptr<SimpleWeb::SocketServerBase<T>::Message> message) {
      auto sub = _server.getSubject();
      auto t = RxWsTask{ conection, message };

      try {
        auto j = json::parse(message->string());
        t.data = make_shared<json>(j);
      } catch (...) {

      }

      t.type = "ON_MESSAGE";
      sub.subscriber().on_next(t);
    };

    ErrorHandler handleError = [this](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
      auto sub = _server.getSubject();
      auto t = RxWsTask{ conection, message };
      json j = {
        { "errorCode": {
            "name": ec.category().name(),
            "value": ec.value()
          }
        }
      };
      t.data = make_shared<json>(t);
      t.type = "ON_ERROR";
      sub.subscriber().on_next(t);
    };

    OpenHandler handleOpen = [this](shared_ptr<WsServer::Connection> connection) {
      auto sub = _server.getSubject();
      auto t = RxWsTask{ conection, message };
      t.type = "ON_OPEN";
      sub.subscriber().on_next(t);
    };

    CloseHandler handleClose = [this](shared_ptr<WsServer::Connection> connection, int status, const string& reason) {
      auto sub = _server.getSubject();
      auto t = RxWsTask{ conection, message };
      json j = {
        { "status": status }
      };
      t.data = make_shared<json>(t);
      t.type = "ON_CLOSE";
      sub.subscriber().on_next(t);
    };


  public:
    // User provides custom Middleware.
    vector<RxWsMiddleware> middlewares;

    // Middleware that will handle the HTTP/S response. 
    RxWsMiddleware onNext;

    // User Defined Endpoints.
    vector<Endpoint<T>> endpoints;

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

    void applyEndpoints() {
      std::for_each(endpoints.begin(), endpoints.end(), [&, this](const Endpoint<T>& r) {
        _server->endpoint[r.expression] = r.action;
      });
    }

    rxweb::subject<T> getSubject() {
      return sub;
    }

    void start() {
      _server->on_open = handleOpen;
      _server->on_message = handleMesssge;
      _server->on_error = handleError;
      _server->on_close = handleClose;
    }

  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType;
    shared_ptr<WsServer> _server;
    rxweb::subject<T> sub;
  };
}
