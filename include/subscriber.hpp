#pragma once

#include "rxweb.hpp"

namespace rxweb {

  template<typename T>
  class subscriber {
  public:
    // using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;

    explicit subscriber() {}

    decltype(auto) create() {
      rxcpp::composite_subscription cs;
      return rxcpp::make_subscriber<RxWebTask>(
        [cs, this](RxWebTask& t) {
        std::cout << "async subscriber thread -> " << hasher(std::this_thread::get_id()) << std::endl;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
      );
    }
  };

  template<typename T>
  class wssubscriber {
  public:
    // using SocketType = SimpleWeb::ServerBase<T>;
    using RxWsTask = rxweb::wstask<T>;

    explicit wssubscriber() {}

    decltype(auto) create() {
      rxcpp::composite_subscription cs;
      return rxcpp::make_subscriber<RxWsTask>(
        [cs, this](RxWsTask& t) {
        std::cout << "async subscriber thread -> " << hasher(std::this_thread::get_id()) << std::endl;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
      );
    }
  };
}
