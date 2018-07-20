#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <rxcpp/rx.hpp>
#include <async++.h>
#include "server_http.hpp"
#include "client_http.hpp"
#include "rxweb/src/rxweb.hpp"
#include "rxweb/src/server.hpp"
#include "rxweb/src/wsserver.hpp"
#include "client_ws.hpp"

using namespace std;
using json = nlohmann::json;

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

std::hash<std::thread::id> hasher;

// https://github.com/eidheim/Simple-WebSocket-Server/blob/master/ws_examples.cpp#L102
shared_ptr<WsClient> makeClient(const string url, const string message) {
  auto client = make_shared<WsClient>(url);

  client->on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::Message> message) {
    auto message_str = message->string();

    cout << "Client: Message received: \"" << message_str << "\"" << endl;

    // cout << "Client: Sending close connection" << endl;
    // connection->send_close(1000);
  };

  client->on_open = [=](shared_ptr<WsClient::Connection> connection) {
    cout << "Client: Opened connection" << endl;

    cout << "Client: Sending message: \"" << message << "\"" << endl;

    auto send_stream = make_shared<WsClient::SendStream>();
    *send_stream << message;
    connection->send(send_stream);
  };

  client->on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
    cout << "Client: Closed connection with status code " << status << endl;
  };

  // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  client->on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
    cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
  };

  return client;
}

int testWebSocketServer() {
  using WebSocketType = SimpleWeb::SocketServerBase<SimpleWeb::WS>;
  using WebSocketTask = rxweb::wstask<SimpleWeb::WS>;

  rxweb::wsserver<SimpleWeb::WS> server(8080, 1);

  server.routes = {
    {
      "^/string/?$",
      [&](std::shared_ptr<WebSocketType::Connection> connection, std::shared_ptr<WebSocketType::Message> message) {
        auto sub = server.getSubject();
        auto t = WebSocketTask{ connection, message };
        auto msg = message->string();
        t.ss = make_shared<stringstream>(msg);

        t.type = "RESPOND";
        sub.subscriber().on_next(t);
      }
    },
    {
      "^/json/?$",
      [&](std::shared_ptr<WebSocketType::Connection> connection, std::shared_ptr<WebSocketType::Message> message) {
        auto sub = server.getSubject();
        auto t = WebSocketTask{ connection, message };
        auto msg = message->string();
        
        try {
          auto j = json::parse(msg.c_str());
          t.data = make_shared<json>(j);
        } catch (...) {
          auto e = R"({"error": "parse error"})"_json;
          t.data = make_shared<json>(e);
        }

        t.type = "RESPOND";
        sub.subscriber().on_next(t);

        server.broadcast("Broadcasting from /json path");
      }
    }
  };

  server.middlewares = {
    {
      [](const WebSocketTask& t)->bool { return (t.type == "RESPOND"); },
      [&server](const WebSocketTask& t) {
    
        auto message_str = (*(t.data)).is_null() ? (*(t.ss)).str() : (*(t.data)).dump(2);

        cout << "Server: Message received: \"" << message_str << "\" from " << t.connection.get() << endl;

        cout << "Server: Sending message \"" << message_str << "\" to " << t.connection.get() << endl;

        auto send_stream = make_shared<WebSocketType::SendStream>();
        *send_stream << message_str;
        // connection->send is an asynchronous function
        t.connection->send(send_stream, [](const SimpleWeb::error_code &ec) {
          if (ec) {
            cout << "Server: Error sending message. " <<
              // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
              "Error: " << ec << ", error message: " << ec.message() << endl;
          }
        });
      }
    },
    {
      [](const WebSocketTask& t)->auto { return t.type == "BROADCAST"; },
      [&server](const WebSocketTask& t) {
        auto send_stream = make_shared<WebSocketType::SendStream>();
        *send_stream << "BROADCASTING";
        
        //for (auto& e : t.endpoint->get_connections()) {
        //  e->send(send_stream, [](const SimpleWeb::error_code &ec) {});
        // }
      }
    }
  };

  thread server_thread([&server]() {
    server.start();
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Get a client.
  auto client_0 = makeClient("localhost:8080/string", "Hello");
  thread client_0_thread([&client_0]() {
    client_0->start();
  });

  // Get a another client.
  auto client_1 = makeClient("localhost:8080/json", R"({"foo": "bar"})");
  thread client_1_thread([&client_1]() {
    client_1->start();
  });

  // Get server thread info
  std::cout << "wsserver thread -> " << hasher(server_thread.get_id()) << std::endl;
  server_thread.join();

  client_0_thread.join();
  client_1_thread.join();

  return 0;
}

int main() {
  {
    return testWebSocketServer();
  }

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
    [](const WebTask& t)->bool { return (t.request->path.rfind("/string") != std::string::npos && t.type == "respond"); },
    [](const WebTask& t) {
      const std::string ok("OK");
      cout << "SIZE " << (*t.ss).str() << endl;;
      *(t.response) << "HTTP/1.1 200 OK\r\nContent-Length: " << ((*(t.ss)).str().size() + ok.length()) << "\r\n\r\n" << ok << (*(t.ss)).str();
    }
  };

  server.middlewares = {
    {
      [](const WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "1"); },
      [&server](const WebTask& t) {
        auto cp = t;

        *(cp.ss) << "1\n";
        cp.type = "2";
        server.getSubject().subscriber().on_next(cp);
      }
    },
    {
      [](const WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "2"); },
      [&server](const WebTask& t) { 
        auto cp = t;
        
        *(cp.ss) << "22\n";
        cp.type = "3";
        server.getSubject().subscriber().on_next(cp);
      }
    },
    {
      [](const WebTask& t)->bool { return (t.request->path.rfind("/json") == std::string::npos && t.type == "3"); },
      [&server](const WebTask& t) {
        auto cp = t;
                
        *(cp.ss) << "333\n";
        cp.type = "4";
        server.getSubject().subscriber().on_next(cp);
      }
    },
    {
      [](const WebTask& t)->bool { return (t.request->path.rfind("/string") == std::string::npos && t.type == "4"); },
      [&server](const WebTask& t) {
        auto cp = t;
        
        *(cp.ss) << "333\n";
        cp.type = "4";
        server.getSubject().subscriber().on_next(cp);
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