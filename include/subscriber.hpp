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
        cout << (*(t.ss)).str() << endl;
        std::cout << "async subscriber thread -> " << hasher(std::this_thread::get_id()) << std::endl;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; },
        []() { cout << "OKOK" << endl; }
      );
    }
  };
}
