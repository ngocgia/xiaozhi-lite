# Các dòng khung chuẩn dưới đây phải nằm trong CMakeLists của dự án
# theo đúng thứ tự này để CMake hoạt động chính xác.
cmake_minimum_required(VERSION 3.16)

# Tắt cảnh báo thiếu field initializer (dùng nhiều struct C của IDF)
add_compile_options(-Wno-missing-field-initializers)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Thu gọn build: chỉ đưa vào main, các component tối thiểu và thành phần phụ thuộc.
idf_build_set_property(MINIMAL_BUILD ON)

# Giữ nguyên version của upstream để tương thích OTA với server Xiaozhi
set(PROJECT_VER "2.5.1")
project(xiaozhi-lite)
