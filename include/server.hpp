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
    
    explicit server(int _port, int _threads) : port(_port), threads(_threads) {
      _server = make_shared<WebServer>(port, threads);
    }

    explicit server(
      int _port, 
      int _threads, 
      const string _certFile, 
      const string _privateKeyFile
      ) : port(_port), threads(_threads), certFile(_certFile), privateKeyFile(_privateKeyFile) {
      // Ignore certs if HTTP is specified.
      if (std::is_same<SocketType, SimpleWeb::HTTP>) {
        _server = make_shared<WebServer>(port, threads);
      } else {
        _server = make_shared<WebServer>(port, threads, certFile, privateKeyFile);
      }
    }
    
    void start() {
      
      // Depending on the observer's filter function, each observer will act or ignore any incoming web request.
      makeObserversFromMiddlewares();

      // Wait for all observers to finish.
      auto subscriber = rxcpp::make_subscriber<RxWebTask>(
        [](RxWebTask& t) { }, //noop,
        [](const std::exception_ptr& e) { std::cout << "Error!" << std::endl; }
      );
      
      // Create a new thread for every chunk {rxweb::task}.
      // Merge then concat will process observables in sequence.
      auto vals = rxcpp::observable<>::iterate(v)
        .merge(RxNewThread)
        .concat()
        .subscribe(subscriber);

      // Defaults: 1 endpoint for POST/GET
      _server->default_resource["POST"] = [this](std::shared_ptr<SocketType::Response> response, std::shared_ptr<SocketType::Request> request) {
        auto t = RxWebTask{ request, response };
        sub.subscriber().on_next(t);
      };

      _server->default_resource["GET"] = [](shared_ptr<SocketType::Response> response, shared_ptr<SocketType::Request> request) {
        *response << "HTTP/1.1 200 OK\r\nContent-Length: " << 0 << "\r\n\r\n";
      };

      _server->start();
    }
  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType;
    shared_ptr<WebServer> _server;
    
    rxweb::subject<T> sub;
    std::vector<rxcpp::observable<RxWebTask>> v;
    
    /*
      Using makeObserversFromMiddlewares() will wait for all middlewares to complete.
    */
    void makeObserversFromMiddlewares() {
      // Create Observers that react to subscriber broadcast.
      std::for_each(middlewares.begin(), middlewares.end(), [&](auto& route) {
        RxWebObserver observer(sub.observable(), route.filterFunc, route.mapFunc);
        v.emplace_back(observer.observable());
      });
      // Last Observer is the one that will respond to client after all middlwares have been processed.
      RxWebObserver lastObserver(sub.observable(), onNext.mapFunc);
      v.emplace_back(lastObserver.observable());
    }

    /*
      Using makeObserversAndSubscribeFromMiddlewares() will not wait for all middlewares to complete.
    */
    void makeObserversAndSubscribeFromMiddlewares() {
      // No subscription, observers does nothing.
      RxWebSubscriber subr;
      auto rxwebSubscriber = subr.create();

      // Create Observers that react to subscriber broadcast.
      std::for_each(middlewares.begin(), middlewares.end(), [&](auto& route) {
        RxWebObserver observer(sub.observable(), route.filterFunc, route.mapFunc);
        observer.subscribe(rxwebSubscriber);
      });
      // Last Observer is the one that will respond to client after all middlwares have been processed.
      RxWebObserver lastObserver(sub.observable(), onNext.mapFunc);
      lastObserver.subscribe(rxwebSubscriber);
    }
  };
}
