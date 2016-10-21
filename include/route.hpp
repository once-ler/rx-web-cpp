#pragma once

#include <functional>
#include "rxweb.hpp"

using FilterFunc = std::function<bool(rxweb::task&)>;
using MapFunc = std::function<rxweb::task&(rxweb::task&)>;

struct Route {
  FilterFunc filterFunc;
  MapFunc mapFunc;
  Route(FilterFunc _filterFunc, MapFunc _mapFunc) : filterFunc(_filterFunc), mapFunc(_mapFunc) {}
};
