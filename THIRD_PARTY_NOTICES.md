# Third-Party Notices

This project uses third-party open-source software.

## 1) nlohmann_json
- Project: nlohmann/json
- Version: 3.11.3
- Source: https://github.com/nlohmann/json
- License: MIT License
- License text: https://github.com/nlohmann/json/blob/v3.11.3/LICENSE.MIT

### How it is used
- JSON parsing for logger configuration loading (`load_config_from_json_file`).
- CMake integrated `FetchContent` in `CMakeLists.txt`.

## Notes
- This file documents dependencies currently referenced by this repository.
- Add additional entries here whenever new external libraries are introduced.
