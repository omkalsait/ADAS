# ADAS
ADAS system finetune for indian roads and depoloyed on Jetson nano.

## Build prerequisites

Install a C++17 compiler, CMake 3.16 or newer, and the OpenCV development
packages for `core` and `videoio`. On Ubuntu:

```sh
sudo apt update
sudo apt install build-essential cmake libopencv-core-dev libopencv-videoio-dev
```

## Configure and build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The `fcw_capture` target currently builds the video capture stage.
