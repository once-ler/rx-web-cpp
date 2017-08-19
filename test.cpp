#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <rxcpp/rx.hpp>
#include <async++.h>
#include "server_http.hpp"
#include "client_http.hpp"
#include "rxweb.hpp"
#include "server.hpp"
#include "wsserver.hpp"
// #include "base.hpp"

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

std::hash<std::thread::id> hasher;

int main() {

  using WebTask = rxweb::task<SimpleWeb::HTTP>;
  using SocketType = SimpleWeb::ServerBase<SimpleWeb::HTTP>;
  
  rxweb::server<SimpleWeb::HTTP> server(8080, 1);
  
  server.routes = {
    {
      "/hl7",
      "POST",
      [&](std::shared_ptr<SocketType::Response> response, std::shared_ptr<SocketType::Request> request) {
        auto sub = server.getSubject();
        auto t = WebTask{ request, response };
        t.type = "parse";
        sub.subscriber().on_next(t);
      }
    }
  };

  server.onNext = {
    [](WebTask& t)->bool { return (t.request->path.rfind("/string") != std::string::npos && t.type == "respond"); },
    [](WebTask& t) {
      const std::string ok("OK");
      cout << "SIZE " << (*t.ss).str() << endl;;
      *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << ((*(t.ss)).str().size() + ok.length()) << "\r\n\r\n" << ok << (*(t.ss)).str();
    }
  };

  server.middlewares = {
    {
      [](WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "1"); },
      [&server](WebTask& t) { 
        *(t.ss) << "1\n";
        t.type = "2";
        server.getSubject().subscriber().on_next(t);
      }
    },
    {
      [](WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "2"); },
      [&server](WebTask& t) { 
        *(t.ss) << "22\n";
        t.type = "3";
        server.getSubject().subscriber().on_next(t);
      }
    },
    {
      [](WebTask& t)->bool { return (t.request->path.rfind("/json") == std::string::npos && t.type == "3"); },
      [&server](WebTask& t) {
        *(t.ss) << "333\n";
        t.type = "4";
        server.getSubject().subscriber().on_next(t);
      }
    },
    {
      [](WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "4"); },
      [&server](WebTask& t) { 
        *(t.ss) << "333\n";
        t.type = "4";
        server.getSubject().subscriber().on_next(t);
      }
    }
  };

  std::thread server_thread([&server]() {
    server.start();    
  });

  // Wait for server to start so that the client can connect
  std::this_thread::sleep_for(std::chrono::seconds(1));
    
  // Tests below
  std::string json_string = "{\"firstName\": \"John\",\"lastName\": \"Smith\",\"age\": 25}";
  
  // Use async++
  async::parallel_for(async::irange(0, 1), [&json_string](int x) {
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