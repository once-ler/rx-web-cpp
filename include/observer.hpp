#pragma once

#include "rxweb.hpp"

namespace rxweb {
  
  template<typename T>
  class observer {
    using SocketType = SimpleWeb::ServerBase<T>;
    using RxWebTask = rxweb::task<T>;
    using FilterFunc = std::function<bool(RxWebTask&)>;
    using MapFunc = std::function<RxWebTask&(RxWebTask&)>;
    using Observable = rxcpp::observable<RxWebTask>;

  public:
    template<typename... Funcs>
    explicit observer(Observable o, FilterFunc filterFunc, MapFunc mapFunc) {
      _observer = o.observe_on(RxEventLoop)
        .filter(filterFunc)
        .map(mapFunc);
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
