cmake_minimum_required(VERSION 3.21)
project(TongChuangQS VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QS_BUILD_GUI "构建 Qt 图形界面" ON)
option(QS_BUILD_TESTS "构建测试" ON)
# 可扩展性演示：-DQS_DISABLED_SERVICES=weshop 表示不编译 services/weshop，平台其余代码不做任何修改
set(QS_DISABLED_SERVICES "" CACHE STRING "不参与构建的服务目录名（分号分隔）")

# 从源文件列表中去掉被禁用服务的文件
function(qs_drop_disabled_services listVar)
  set(result ${${listVar}})
  foreach(name ${QS_DISABLED_SERVICES})
    list(FILTER result EXCLUDE REGEX "/services/${name}/")
  endforeach()
  set(${listVar} ${result} PARENT_SCOPE)
endfunction()

if(MSVC)
  add_compile_options(/utf-8 /W4 /permissive- /EHsc /wd4100)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00 _CRT_SECURE_NO_WARNINGS)
endif()

# ———— 服务目录：services/<服务名>/ ————
# 新增服务只需在 services 下新建一个目录：它的 .cpp 被自动纳入，平台代码无需修改。
file(GLOB QS_SERVICE_DIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/services/*")
set(QS_SERVICE_INCLUDES "")
foreach(dir ${QS_SERVICE_DIRS})
  get_filename_component(serviceName "${dir}" NAME)
  if(IS_DIRECTORY "${dir}" AND NOT serviceName IN_LIST QS_DISABLED_SERVICES)
    list(APPEND QS_SERVICE_INCLUDES "${dir}")
  endif()
endforeach()

file(GLOB QS_CORE_SOURCES CONFIGURE_DEPENDS
  common/*.cpp platform/*.cpp components/*.cpp chat/*.cpp client/*.cpp services/*/*.cpp)
qs_drop_disabled_services(QS_CORE_SOURCES)
if(QS_DISABLED_SERVICES)
  message(STATUS "本次构建不包含服务：${QS_DISABLED_SERVICES}")
endif()

# OBJECT 库：所有目标文件都会进入可执行文件，保证各服务的自注册静态对象不被链接器丢弃。
add_library(CoreQS OBJECT ${QS_CORE_SOURCES})
target_include_directories(CoreQS PUBLIC common platform components chat client ${QS_SERVICE_INCLUDES})
target_include_directories(CoreQS SYSTEM PUBLIC third_party)
if(WIN32)
  target_link_libraries(CoreQS PUBLIC ws2_32 shell32)   # Winsock 2
else()
  find_package(Threads REQUIRED)
  target_link_libraries(CoreQS PUBLIC Threads::Threads)
endif()

# ———— 控制台版 ————
file(GLOB QS_CONSOLE_SOURCES CONFIGURE_DEPENDS console/*.cpp services/*/console/*.cpp)
qs_drop_disabled_services(QS_CONSOLE_SOURCES)
add_executable(SocialQS ${QS_CONSOLE_SOURCES})
target_include_directories(SocialQS PRIVATE console)
target_link_libraries(SocialQS PRIVATE CoreQS)

# ———— 图形界面版 ————
if(QS_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
  set(CMAKE_AUTOMOC ON)
  file(GLOB QS_GUI_SOURCES CONFIGURE_DEPENDS gui/*.cpp gui/*.h services/*/gui/*.cpp services/*/gui/*.h)
  qs_drop_disabled_services(QS_GUI_SOURCES)
  set(QS_GUI_EXTRA "")
  if(WIN32)
    list(APPEND QS_GUI_EXTRA resources/TongChuangQS.rc)
  endif()
  add_executable(TongChuangQS WIN32 ${QS_GUI_SOURCES} ${QS_GUI_EXTRA})
  target_include_directories(TongChuangQS PRIVATE gui)
  target_link_libraries(TongChuangQS PRIVATE CoreQS Qt6::Widgets)
endif()

# ———— 测试 ————
if(QS_BUILD_TESTS)
  enable_testing()
  add_executable(ServiceSmokeQS tests/ServiceSmokeQS.cpp)
  target_include_directories(ServiceSmokeQS PRIVATE tests)
  target_link_libraries(ServiceSmokeQS PRIVATE CoreQS)
  if("weshop" IN_LIST QS_DISABLED_SERVICES)
    target_compile_definitions(ServiceSmokeQS PRIVATE QS_EXPECT_NO_WESHOP)
  endif()
  add_test(NAME service-smoke COMMAND ServiceSmokeQS)
endif()
if(QS_BUILD_TESTS AND NOT QS_DISABLED_SERVICES)   # 完整核心测试按四个服务编写
  add_executable(CoreTestsQS tests/CoreTestsQS.cpp)
  target_include_directories(CoreTestsQS PRIVATE tests)
  target_link_libraries(CoreTestsQS PRIVATE CoreQS)
  add_test(NAME core COMMAND CoreTestsQS)
  add_executable(ChatTestsQS tests/ChatTestsQS.cpp)
  target_include_directories(ChatTestsQS PRIVATE tests)
  target_link_libraries(ChatTestsQS PRIVATE CoreQS)
  add_test(NAME chat COMMAND ChatTestsQS)
  add_executable(DeliveryRegressionQS tests/DeliveryRegressionQS.cpp)
  target_include_directories(DeliveryRegressionQS PRIVATE tests)
  target_link_libraries(DeliveryRegressionQS PRIVATE CoreQS)
  add_test(NAME delivery-regression COMMAND DeliveryRegressionQS)
endif()
