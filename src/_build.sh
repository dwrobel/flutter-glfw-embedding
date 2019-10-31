#!/bin/bash
EMBEDDER_INC=/home/sw/projects/flutter/dw-3/engine/src/out/linux_debug_unopt_x64
FLUTTER_LIBDIR=/home/sw/projects/flutter/dw-3/engine/src/out/linux_debug_unopt_x64

g++ -O0 -ggdb3 -I${EMBEDDER_INC} -L${FLUTTER_LIBDIR} -lflutter_engine -o flutter.bin FlutterEmbedderGLFW.cc  $(pkg-config --cflags --libs glfw3)