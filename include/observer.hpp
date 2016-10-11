#pragma once

#include "rxweb.hpp"

using Observable = rxcpp::observable<rxweb::task>;

namespace rxweb {
  
  class observer {
  public:
    explicit observer(Observable o) {
      _observer = o.observe_on(RxEventLoop)
        .filter([](rxweb::task& t) { std::cout << " -> filter" << std::endl; return true; })
        .map([](rxweb::task& t) { std::cout << " -> map" << std::endl; t.traceIds.push_back(1); return t; })
        .as_dynamic();
    }

    template<class Arg0>
    decltype(auto) subscribe(Arg0&& a0) {
      _observer.subscribe(a0);
    }

    decltype(auto) observable() {
      return _observer;
    }

  private:
    Observable _observer;
  };
}
