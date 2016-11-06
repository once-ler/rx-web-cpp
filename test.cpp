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

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

std::hash<std::thread::id> hasher;

int main() {

  using WebTask = rxweb::task<SimpleWeb::HTTP>;

  using MapFunc = std::function<WebTask&(WebTask&)>;

  rxweb::server<SimpleWeb::HTTP> server(8080, 1);
  
  server.onNext = {
    [](WebTask& t)->WebTask& {
      const std::string ok("OK");
      cout << "SIZE " << (*t.ss).str() << endl;;
      *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << ((*(t.ss)).str().size() + ok.length()) << "\r\n\r\n" << ok << (*(t.ss)).str();
      return t;
    }
  };

  server.middlewares = {
    {
      [](WebTask& t)->bool { if (t.request->path.rfind("/string") == std::string::npos) return false; return true; },
      [](WebTask& t)->WebTask& { *(t.ss) << "1\n"; return t; }
    },
    {
      [](WebTask& t)->bool { if (t.request->path.rfind("/string") == std::string::npos) return false; return true; },
      [](WebTask& t)->WebTask& { 
        /*
        o.concat_map(
          [](RxWebTask t) { return rxcpp::observable<>::from<RxWebTask>(t); },
          [](RxWebTask t, RxWebTask s) { cout << "COUNT\n"; return t; }
        ).subscribe([](RxWebTask t) {cout << "DONE\n"; });
        */
        *(t.ss) << "22\n"; 
        // *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << 2 << "\r\n\r\nOK";
        return t; 
      }
    },
    {
      [](WebTask& t)->bool { if (t.request->path.rfind("/json") == std::string::npos) return false; return true; },
      [](WebTask& t)->WebTask& {*(t.ss) << "333\n"; return t; }
    },
    {
      [](WebTask& t)->bool { if (t.request->path.rfind("/string") == std::string::npos) return false; return true; },
      [](WebTask& t)->WebTask& { *(t.ss) << "4444\n"; return t; }
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