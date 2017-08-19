#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <rxcpp/rx.hpp>
#include "server_http.hpp"
#include "rxweb.hpp"
#include "subject.hpp"
#include "observer.hpp"
#include "subscriber.hpp"

namespace rxweb {
  template<typename T>
  struct Route {
    using WebAction = std::function<void(shared_ptr<typename SimpleWeb::ServerBase<T>::Response>, shared_ptr<typename SimpleWeb::ServerBase<T>::Request>)>;

    string expression;
    string verb;
    WebAction action;
    Route(string expression_, string verb_, WebAction action_) : expression(expression_), verb(verb_), action(action_) {}
  };

  template<typename T>
  class server {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using RxWebObserver = rxweb::observer<T>;
    using RxWebSubscriber = rxweb::subscriber<T>;
    using RxWebMiddleware = rxweb::middleware<T>;
    using WebServer = SimpleWeb::Server<T>;

  public:
    // User provides custom Middleware.
    vector<RxWebMiddleware> middlewares;

    // Middleware that will handle the HTTP/S response. 
    RxWebMiddleware onNext;

    // User Defined Routes.
    vector<Route<T>> routes;
    
    explicit server(int _port, int _threads = 1) : port(_port), threads(_threads) {
      _server = make_shared<WebServer>();
      _server->config.port = port;
      _server->config.thread_pool_size = threads;
    }

    explicit server(
      int _port, 
      int _threads, 
      const string _certFile, 
      const string _privateKeyFile
      ) : port(_port), threads(_threads), certFile(_certFile), privateKeyFile(_privateKeyFile) {
      // Ignore certs if HTTP is specified.
      if (std::is_same<SocketType, SimpleWeb::HTTP>) {
        _server = make_shared<WebServer>();        
      } else {
        _server = make_shared<WebServer>(certFile, privateKeyFile);
      }
      _server->config.port = port;
      _server->config.thread_pool_size = threads;
    }
    
    void applyRoutes() {
      std::for_each(routes.begin(), routes.end(), [&, this](const Route<T>& r) {
        _server->resource[r.expression][r.verb] = r.action;
      });
    }

    rxweb::subject<T> getSubject() {
      return sub;
    }

    void start() {
      
      // Depending on the observer's filter function, each observer will act or ignore any incoming web request.
      makeObserversAndSubscribeFromMiddlewares();

      // Wait for all observers to finish.
      auto subscriber = rxcpp::make_subscriber<RxWebTask>(
        [](RxWebTask& t) { }, //noop,
        [](const std::exception_ptr& e) { std::cout << "Error!" << std::endl; }
      );
      
      // Defaults: 1 endpoint for POST/GET
      _server->default_resource["POST"] = [this](std::shared_ptr<SocketType::Response> response, std::shared_ptr<SocketType::Request> request) {
        auto t = RxWebTask{ request, response };
        sub.subscriber().on_next(t);
      };

      _server->default_resource["GET"] = [](shared_ptr<SocketType::Response> response, shared_ptr<SocketType::Request> request) {
        *response << "HTTP/1.1 200 OK\r\nContent-Length: " << 0 << "\r\n\r\n";
      };

      // Apply user-defined routes
      applyRoutes();

      _server->start();
    }
  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType;
    shared_ptr<WebServer> _server;
    rxweb::subject<T> sub;
    
    /*
      Using makeObserversAndSubscribeFromMiddlewares() will not wait for all middlewares to complete.
    */
    void makeObserversAndSubscribeFromMiddlewares() {
      // No subscription, observers does nothing.      
      // Create Observers that react to subscriber broadcast.
      std::for_each(middlewares.begin(), middlewares.end(), [&](auto& route) {
        RxWebObserver observer(sub.observable(), route.filterFunc);
        observer.subscribe(route.subscribeFunc);
      });
      // Last Observer is the one that will respond to client after all middlwares have been processed.
      RxWebObserver lastObserver(sub.observable(), onNext.filterFunc);
      lastObserver.subscribe(onNext.subscribeFunc);
    }
  };
}
