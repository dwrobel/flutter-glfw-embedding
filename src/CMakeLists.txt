# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
#

cmake_minimum_required (VERSION 3.3)
project (flutter_lanucher_glfw)
find_package(PkgConfig)

if (CMAKE_VERSION VERSION_LESS "3.8.2")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(SOURCES FlutterEmbedderGLFW.cc)

pkg_search_module(GLFW glfw3 REQUIRED)
# If you do not have flutter-engine.pc file
# just provide all FLUTTER_* variables using -D
pkg_search_module(FLUTTER flutter-engine)

link_directories(${GLFW_LIBRARY_DIRS} ${FLUTTER_LIBRARY_DIRS})

add_executable(flutter-launcher-glfw ${SOURCES})
target_compile_definitions(flutter-launcher-glfw PRIVATE "-DGLFW_INCLUDE_ES2")
target_include_directories(flutter-launcher-glfw PRIVATE ${FLUTTER_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS}})
target_link_libraries(flutter-launcher-glfw ${FLUTTER_LIBRARIES} ${GLFW_LIBRARIES})

install(TARGETS flutter-launcher-glfw DESTINATION bin)
