efyj's todo list
================

- Improve `struct efyj::context` :
  - `efyj::context::log(efyj::context::message)`. Default into the std::log
    and thread safe.
    - `efyj::context::message(int pid, int thread_id, int priority, char
      *format, va_args)`
  - `context::dbg(char *fn, char *fn, int line, char *format, va_args)`.
    Default into std::cerr and can be remove if NDEBUG is enabled.

- Cleanup API, remove dead code, simplify algorithms.
  - `efyj::Model` is complex.

- Switch internal source code into C++11 template.
  - C++11 template for libexpat or libxml2 SAX API.
  - Build a source only header library.