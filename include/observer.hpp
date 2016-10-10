#pragma once

#include <rxcpp/rx.hpp>
#include "rxweb.hpp"

using observable = rxcpp::observable<rxweb::task>;
using subscriber = rxcpp::observable<rxweb::task>;

namespace rxweb {

  class observer {
  public:
    explicit observer() {      
      
    }
    rx::observable<rxweb::task> observe_on(observable o) {
      // observers uses threadpool
      // o.subscribe(subscriber);
      // auto w = o.observe_on(threads).as_dynamic();
      auto threads = rx::observe_on_event_loop();
      _observer = o.observe_on(threads)
        .filter([](rxweb::task& t) { std::cout << " -> filter" << std::endl; return true; })
        .map([](rxweb::task& t) { std::cout << " -> map" << std::endl; t.traceIds.push_back(1); return t; })
        .as_dynamic();
      return _observer;
    }
  private:
    rx::observable<rxweb::task> _observer;
  };
}
