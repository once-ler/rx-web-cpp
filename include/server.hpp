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

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace rx = rxcpp;
namespace rxsub = rxcpp::subjects;

namespace rxweb {
  class server {
  public:
    std::vector<Route> routes;

    server(vector<Route> _routes) : routes(_routes) {
      // Create subscriber to act as proxy to incoming web request.
      // Subscriber will broadcast to observers.
      rxwebSubscriber = subr.create();

      createObserverAll();
    }

    HttpServer create() {
      // wait for all to finish
      rx::composite_subscription cs;
      auto subscriber = rx::make_subscriber<rxweb::task>(
        [&cs](rxweb::task& t) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // std::cout << t.request->content.string() << std::endl;
        std::cout << "last async thread -> " << hasher(std::this_thread::get_id()) << std::endl;
        std::copy(t.traceIds.begin(), t.traceIds.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
        const std::string ok("OK");
        *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << ok.length() << "\r\n\r\n" << ok;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
      );

      // create a new thread for every chunk {rxweb::task}
      rx::observable<>::iterate(v)
        .concat(rx::observe_on_event_loop())
        //.concat(rx::observe_on_new_thread())
        //.as_blocking()
        .subscribe(subscriber);

      HttpServer server(8080, 1);

      server.default_resource["POST"] = [&server, this](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
        auto t = rxweb::task{ request, response };
        sub.subscriber().on_next(t);
      };

      return server;
    }

    void addRoute(Route route) {
      routes.emplace_back(route);
      createObserver(route);
    }

  private:
    rxweb::subject sub;

    rxweb::subscriber subr;
    decltype(subr.create()) rxwebSubscriber;
    // This vector holds all observables that we must wait for before responding to client. 
    std::vector<rx::observable<rxweb::task>> v;

    // Create observers, they will act like route middlware.
    void createObserverAll() {
      std::for_each(routes.begin(), routes.end(), [&](auto& route) {
        rxweb::observer observer(sub.observable(), route.filterFunc, route.mapFunc);
        observer.subscribe(rxwebSubscriber);
        v.emplace_back(observer.observable());
      });
    }

    void createObserver(Route route) {
      rxweb::observer observer(sub.observable(), route.filterFunc, route.mapFunc);
      observer.subscribe(rxwebSubscriber);
      v.emplace_back(observer.observable());
    }

  };
}
