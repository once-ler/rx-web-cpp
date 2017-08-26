#pragma once

#include "rxweb.hpp"
#include "observer.hpp"
#include "subscriber.hpp"

namespace rxweb {
  
  template<typename T>
  class subject {
  public:
    // using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using Subject = rxcpp::subjects::subject<RxWebTask>;

    explicit subject() {
      // Create subject and subscribe on threadpool and make it "hot" immediately
      auto o = sub.get_observable();
      o.subscribe_on(RxEventLoop)
        .publish()
        .as_dynamic();
    }

    decltype(auto) observable() { return sub.get_observable(); }
    
    decltype(auto) subscriber() { return sub.get_subscriber(); }

    decltype(auto) get() { return sub; }

  private:
    Subject sub;
  };

  template<typename T>
  class wssubject {
  public:
    using RxWsTask = rxweb::wstask<T>;
    using Subject = rxcpp::subjects::subject<RxWsTask>;

    explicit wssubject() {
      // Create subject and subscribe on threadpool and make it "hot" immediately
      auto o = sub.get_observable();
      o.subscribe_on(RxEventLoop)
        .publish()
        .as_dynamic();
    }

    decltype(auto) observable() { return sub.get_observable(); }

    decltype(auto) subscriber() { return sub.get_subscriber(); }

    decltype(auto) get() { return sub; }

  private:
    Subject sub;
  };

}
