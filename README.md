# CMake library


project structure
```text
mace/
├── CMakeLists.txt            # Main CMake file
├── cmake/
│   ├── mace-config.cmake.in  # Config file template
│   └── FindDependencies.cmake # Helper for finding dependencies
├── include/
│   └── mace/                 # Public headers
│       ├── mace.hpp
│       └── ...
├── src/                      # Implementation files
│   ├── core.cpp
│   └── ...
└── examples/                 # Optional example code
    ├── CMakeLists.txt
    └── simple_example.cpp
```