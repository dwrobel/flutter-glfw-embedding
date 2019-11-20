//
// Copyright 2019 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Initial version is based on chinmaygarde's github gist
// (see git log for full url)
//

#include <fstream>
#include <assert.h>
#include <chrono>
#include <flutter_embedder.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <linux/limits.h>
#include <unistd.h>

static_assert(FLUTTER_ENGINE_VERSION == 1, "");

static const size_t kInitialWindowWidth  = 800;
static const size_t kInitialWindowHeight = 600;

// Returns the path of the directory containing this executable, or an empty
// string if the directory cannot be found.
std::string GetExecutableDirectory() {
  char buffer[PATH_MAX + 1];
  ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (length > PATH_MAX) {
    std::cerr << "Couldn't locate executable" << std::endl;
    return "";
  }
  std::string executable_path(buffer, length);
  size_t last_separator_position = executable_path.find_last_of('/');
  if (last_separator_position == std::string::npos) {
    std::cerr << "Unabled to find parent directory of " << executable_path << std::endl;
    return "";
  }
  return executable_path.substr(0, last_separator_position);
}

void GLFWcursorPositionCallbackAtPhase(GLFWwindow *window, FlutterPointerPhase phase, double x, double y) {
  FlutterPointerEvent event = {};
  event.struct_size         = sizeof(event);
  event.phase               = phase;
  event.x                   = x;
  event.y                   = y;
  event.timestamp           = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  FlutterEngineSendPointerEvent(reinterpret_cast<FlutterEngine>(glfwGetWindowUserPointer(window)), &event, 1);
}

void GLFWcursorPositionCallback(GLFWwindow *window, double x, double y) {
  GLFWcursorPositionCallbackAtPhase(window, FlutterPointerPhase::kMove, x, y);
}

void GLFWmouseButtonCallback(GLFWwindow *window, int key, int action, int mods) {
  if (key == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    GLFWcursorPositionCallbackAtPhase(window, FlutterPointerPhase::kDown, x, y);
    glfwSetCursorPosCallback(window, GLFWcursorPositionCallback);
  }

  if (key == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    GLFWcursorPositionCallbackAtPhase(window, FlutterPointerPhase::kUp, x, y);
    glfwSetCursorPosCallback(window, nullptr);
  }
}

static void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

void GLFWwindowSizeCallback(GLFWwindow *window, int width, int height) {
  FlutterWindowMetricsEvent event = {};
  event.struct_size               = sizeof(event);
  event.width                     = width;
  event.height                    = height;
  event.pixel_ratio               = 1.0;
  FlutterEngineSendWindowMetricsEvent(reinterpret_cast<FlutterEngine>(glfwGetWindowUserPointer(window)), &event);
}

bool RunFlutter(GLFWwindow *window) {
  FlutterRendererConfig config = {};
  config.type                  = kOpenGL;
  config.open_gl.struct_size   = sizeof(config.open_gl);
  config.open_gl.make_current  = [](void *userdata) -> bool {
    glfwMakeContextCurrent((GLFWwindow *)userdata);
    return true;
  };
  config.open_gl.clear_current = [](void *) -> bool {
    glfwMakeContextCurrent(nullptr); // is this even a thing?
    return true;
  };
  config.open_gl.present = [](void *userdata) -> bool {
    glfwSwapBuffers((GLFWwindow *)userdata);
    return true;
  };
  config.open_gl.fbo_callback = [](void *) -> uint32_t {
    return 0; // FBO0
  };

  config.open_gl.gl_proc_resolver = [](void *userdata, const char *name) -> void * {
    auto address = glfwGetProcAddress(name);
    if (address != nullptr) {
      return reinterpret_cast<void *>(address);
    }
    std::cout << "Tried unsuccessfully to resolve: " << name << std::endl;
    return nullptr;
  };

  FlutterProjectArgs args = {
      .struct_size = sizeof(FlutterProjectArgs),
  };

  // Resources are located relative to the executable.
  std::string base_directory = GetExecutableDirectory();
  if (base_directory.empty()) {
    base_directory = ".";
  }
  std::string data_directory = base_directory + "/data";
  std::string assets_path    = data_directory + "/flutter_assets";
  std::string icu_data_path  = data_directory + "/icudtl.dat";

  do {
    if (std::ifstream(icu_data_path)) {
      std::cout << "Using: " << icu_data_path << std::endl;
      break;
    }

    icu_data_path = "/usr/share/flutter/icudtl.dat";

    if (std::ifstream(icu_data_path)) {
      std::cout << "Using: " << icu_data_path << std::endl;
      break;
    }

    std::cerr << "Unnable to locate icudtl.dat file" << std::endl;
    return EXIT_FAILURE;
  } while (0);

  args.icu_data_path = icu_data_path.c_str();

  do {
    if (std::ifstream(assets_path)) {
      std::cout << "Using: " << assets_path << std::endl;
      break;
    }

    std::cerr << "Unnable to locate assets_path directory in: " << assets_path << std::endl;
    return EXIT_FAILURE;
  } while (0);

  args.assets_path = assets_path.c_str();

  FlutterEngine engine = nullptr;
  auto result          = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, // renderer
                                 &args, window, &engine);
  assert(result == kSuccess && engine != nullptr);

  glfwSetWindowUserPointer(window, engine);

  GLFWwindowSizeCallback(window, kInitialWindowWidth, kInitialWindowHeight);

  return true;
}

int main(int argc, const char *argv[]) {

  auto result = glfwInit();
  assert(result == GLFW_TRUE);

  // Prefer EGL API
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  // Prefer GLESv2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);

  auto window = glfwCreateWindow(kInitialWindowWidth, kInitialWindowHeight, "Flutter", NULL, NULL);
  assert(window != nullptr);

  bool runResult = RunFlutter(window);
  assert(runResult);

  glfwSetKeyCallback(window, GLFWKeyCallback);

  glfwSetWindowSizeCallback(window, GLFWwindowSizeCallback);

  glfwSetMouseButtonCallback(window, GLFWmouseButtonCallback);

  while (!glfwWindowShouldClose(window)) {
    //    std::cout << "Looping..." << std::endl;
    glfwWaitEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
