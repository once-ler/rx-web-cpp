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

  rxweb::server<SimpleWeb::HTTP> server(8080, 1);
  server.routes = {
    {
      [](WebTask& t)->bool { std::cout << t.request->path << std::endl; if (t.request->path.rfind("/string") == std::string::npos) return false; return true; },
      [](WebTask& t)->WebTask& { std::cout << "aaa\n"; return t; }
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