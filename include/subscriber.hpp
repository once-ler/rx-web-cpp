#pragma once

#include "rxweb.hpp"

namespace rxweb {

  class subscriber {
  public:
    explicit subscriber() {}
    explicit subscriber(int s) : i_(s) {

    }

    decltype(auto) create() {
      rxcpp::composite_subscription cs;
      return rxcpp::make_subscriber<rxweb::task>(
        [cs, this](rxweb::task& t) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << i_ << " subscriber" << std::endl;
        std::cout << "async subscriber thread -> " << hasher(std::this_thread::get_id()) << std::endl;
      },
        [](const std::exception_ptr& e) { std::cout << "error." << std::endl; }
      );
    }

  private:
    int i_;
  };

}