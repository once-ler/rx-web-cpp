rx-web-cpp
==========

#### A web and WebSocket server powered by [RxCpp](https://github.com/Reactive-Extensions/RxCpp)

__Warning: This is experimental and WIP!__
##### Implementation

* One endpoint.
* All requests must be HTTP POST.
* One Subject as Router and act as dispatcher.
* Routes are simply subscribers.
* Server run on one thread, subscribers run on Scheduler thread pool.
