#pragma once

#include "rxweb.hpp"

namespace rxweb {
  
  template<typename T>
  class observer {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using FilterFunc = std::function<bool(RxWebTask&)>;
    using Observable = rxcpp::observable<RxWebTask>;

  public:
    explicit observer(Observable o, FilterFunc filterFunc) {
      _observer = o.observe_on(RxEventLoop)
        .filter(filterFunc);
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

  template<typename T>
  class wsobserver {
    using SocketType = SimpleWeb::SocketServerBase<T>;
    using RxWsTask = rxweb::wstask<T>;
    using FilterFunc = std::function<bool(RxWsTask&)>;
    using Observable = rxcpp::observable<RxWsTask>;

  public:
    explicit wsobserver(Observable o, FilterFunc filterFunc) {
      _observer = o.observe_on(RxEventLoop)
        .filter(filterFunc);
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
