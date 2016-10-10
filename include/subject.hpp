#pragma once

#include <rxcpp/rx.hpp>
#include "rxweb.hpp"

namespace rx = rxcpp;
namespace rxsub = rxcpp::subjects;

namespace rxweb {
  
  class subject {
  public:
    explicit subject() {
      // Create subject and subscribe on threadpool and make it "hot" immediately
      auto threads = rx::observe_on_event_loop();      
      // auto s = sub.get_subscriber();
      auto o = sub.get_observable();
      o.subscribe_on(threads)
        .publish()
        .as_dynamic();
    }
    rxcpp::observable<rxweb::task> observable() { return sub.get_observable(); }
    rxcpp::subscriber<rxweb::task> subscriber() { return sub.get_subscriber(); }
  private:
    rxsub::subject<rxweb::task> sub;
  };

}
