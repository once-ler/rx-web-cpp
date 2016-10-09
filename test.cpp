#pragma once

/*
To stop boost from complaining: Unknown compiler version
go to boost/config/compiler/visualc.hpp and replace:
#if (_MSC_VER > 1800 && _MSC_FULL_VER > 190024210)
or download:
https%3A%2F%2Fsourceforge.net%2Fprojects%2Fboost%2Ffiles%2Fboost-binaries%2F1.62.0%2F&ts=1476039395&use_mirror=pilotfiber
*/
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <rxcpp/rx.hpp>
#include <async++.h>
#include "server_http.hpp"
#include "client_http.hpp"
#include "ioc.hpp"
#include "rxweb.hpp"
#include "subject.hpp"
#include "observer.hpp"
#include "subscriber.hpp"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

std::hash<std::thread::id> hasher;

namespace rx = rxcpp;
namespace rxsub = rxcpp::subjects;

int main() {

  IOCContainer container;
  auto subject = container.GetInstance<rxweb::subject>();

  // Create subject and subscribe on threadpool and make it "hot" immediately
  auto threads = rx::observe_on_event_loop();
  rxsub::subject<rxweb::task> sub;
  auto s = sub.get_subscriber();  
  auto o = sub.get_observable();
  o.subscribe_on(threads)
    .publish()
    .as_dynamic();
  
  // // factory pattern to consider?
  // typedef rxcpp::resource<std::vector<int>> resource;
  // auto resource_factory = []() {return resource(rxcpp::util::to_vector({ 1, 2, 3, 4, 5 })); };
  std::vector<rx::observable<rxweb::task>> v;
  
  for (int s = 0; s < 5; s++) {
    rx::composite_subscription cs;
    auto subscriber = rx::make_subscriber<rxweb::task>(
      [cs, s](rxweb::task& t) {
      // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << s << " subscriber" << std::endl;
      std::cout << "async subscriber thread -> " << hasher(std::this_thread::get_id()) << std::endl;
    },
      [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
    );

    // observers uses threadpool
    // o.subscribe(subscriber);
    // auto w = o.observe_on(threads).as_dynamic();
    auto observer = o.observe_on(threads)
      .filter([](rxweb::task& t) { std::cout << " -> filter" << std::endl; return true; })
      .map([](rxweb::task& t) { std::cout << " -> map" << std::endl; t.traceIds.push_back(1); return t; })
      .as_dynamic();

    observer.subscribe(subscriber);
    v.push_back(observer);
  }

  // wait for all to finish
  rx::composite_subscription cs;
  auto subscriber = rx::make_subscriber<rxweb::task>(
    [cs](rxweb::task& t) {
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
    .concat(rx::observe_on_new_thread())
    //.as_blocking()
    .subscribe(subscriber);
  
  HttpServer server(8080, 1);

  server.default_resource["POST"] = [&server, &sub](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    auto s = sub.get_subscriber();
    auto t = rxweb::task{ request, response };
    s.on_next(t);
  };

  std::thread server_thread([&server]() {
    server.start();    
  });

  // Wait for server to start so that the client can connect
  std::this_thread::sleep_for(std::chrono::seconds(1));
    
  // Tests below
  std::string json_string = "{\"firstName\": \"John\",\"lastName\": \"Smith\",\"age\": 25}";
  
  // Use async++
  async::parallel_for(async::irange(0, 100), [&json_string](int x) {
    try {
      HttpClient client("localhost:8080");
      auto r2 = client.request("POST", "/string", json_string);
      std::cout << "Request " << x << " -> " << r2->content.rdbuf() << "\r\n\r\n";      
    } catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  });
  
  // Get server thread info
  std::cout << "server thread -> " << hasher(server_thread.get_id()) << std::endl;
  server_thread.join();
  
  return 0;
}