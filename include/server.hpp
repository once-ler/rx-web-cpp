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

namespace rx = rxcpp;
namespace rxsub = rxcpp::subjects;

namespace rxweb {
  
  template<typename T>
  class server {
  public:
    // User provides custom Route
    vector<rxweb::Route> routes;

    explicit server<T>(int _port, int _threads) : port(_port), threads(_threads) {
      _server = make_shared<SimpleWeb::Server<T>>(port, threads);
    }

    explicit server<T>(
      int _port, 
      int _threads, 
      const string _certFile, 
      const string _privateKeyFile
      ) : port(_port), threads(_threads), certFile(_certFile), privateKeyFile(_privateKeyFile) {
      // Ignore certs if HTTP is specified.
      if (std::is_same<T, SimpleWeb::HTTP>) {
        _server = make_shared<SimpleWeb::Server<T>>(port, threads);
      } else {
        _server = make_shared<SimpleWeb::Server<T>>(port, threads, sertFile, privateKeyFile);
      }
    }
    
    void start() {
      // Depending on the observer's filter function, each observer will act or ignore any incoming web request.
      makeObserversFromRoutes();

      // Wait for all observers to finish.
      rx::composite_subscription cs;
      auto subscriber = rx::make_subscriber<rxweb::task>(
        [&cs](rxweb::task& t) {
        
        const std::string ok("OK");
        *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << (t.response->size() + ok.length()) << "\r\n\r\n" << ok;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
      );

      // create a new thread for every chunk {rxweb::task}
      rx::observable<>::iterate(v)
        .concat(rx::observe_on_event_loop())
        //.concat(rx::observe_on_new_thread())
        .subscribe(subscriber);

      // Defaults: 1 endpoint for POST/GET
      _server->default_resource["POST"] = [this](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
        auto t = rxweb::task{ request, response };
        sub.subscriber().on_next(t);
      };

      _server->default_resource["GET"] = [](shared_ptr<SimpleWeb::ServerBase<T>::Response> response, shared_ptr<SimpleWeb::ServerBase<T>::Request> request) {
        *response << "HTTP/1.1 200 OK\r\nContent-Length: " << 0 << "\r\n\r\n";
      };

      _server->start();
    }
  private:
    int port, threads;
    std::string certFile, privateKeyFile, socketType;
    shared_ptr<SimpleWeb::Server<T>> _server;
    
    rxweb::subject sub;
    std::vector<rx::observable<rxweb::task>> v;

    void makeObserversFromRoutes() {
      // Create subscriber to act as proxy to incoming web request.
      // Subscriber will broadcast to observers.
      rxweb::subscriber subr;
      auto rxwebSubscriber = subr.create();

      // Create Observers that react to subscriber broadcast.
      std::for_each(routes.begin(), routes.end(), [&](auto& route) {
        rxweb::observer observer(sub.observable(), route.filterFunc, route.mapFunc);
        observer.subscribe(rxwebSubscriber);
        v.emplace_back(observer.observable());
      });
    }
  };
}
