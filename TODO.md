efyj's todo list
================

- Cleanup: All thrown exceptions should use `efyj_error` class.
- Cleanup: remove dead code, simplify algorithms.
- Add option to unread OPTION in DEXi files.
- Switch internal source code into C++11 template.
  - C++11 template for libexpat or libxml2 SAX API.
- To avoid any cast, replace all `std::size_t` from `scale_size` with int or
  `scale_id`.
- Replace `scale_size()` with `scale_result()`.
- Homogeneize read/write with `std::[io]stream` and `operator<<`.