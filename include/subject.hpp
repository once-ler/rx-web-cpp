#pragma once

#include <rxcpp/rx.hpp>
#include "rxweb.hpp"
#include "ioc.hpp"

namespace rx = rxcpp;
namespace rxsub = rxcpp::subjects;

namespace rxweb {
  
  class subject {
  public:
    explicit subject() {
      // Create subject and subscribe on threadpool and make it "hot" immediately
      auto threads = rx::observe_on_event_loop();
      rxsub::subject<rxweb::task> sub;
      auto s = sub.get_subscriber();
      auto o = sub.get_observable();
      o.subscribe_on(threads)
        .publish()
        .as_dynamic();
    }
  };

  auto registerSubject = []() {
    IOCContainer container;
    container.RegisterSingletonClass<rxweb::subject>();
  };
  
}
