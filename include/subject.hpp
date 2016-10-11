#pragma once

#include "rxweb.hpp"

using Subject = rxcpp::subjects::subject<rxweb::task>;

namespace rxweb {
  
  class subject {
  public:
    explicit subject() {
      // Create subject and subscribe on threadpool and make it "hot" immediately
      auto o = sub.get_observable();
      o.subscribe_on(RxEventLoop)
        .publish()
        .as_dynamic();
    }

    decltype(auto) observable() { return sub.get_observable(); }
    
    decltype(auto) subscriber() { return sub.get_subscriber(); }
  
  private:
    Subject sub;
  };

}
