# optimi-cpp-logger

A lightweight C++ logger for writing application logs to files from anywhere in your codebase.


## Logger Configuration Parameters

The logger uses `LoggerConfig` parameters and can also load them from a JSON file.

### `log_file_path` (string, required)
- Path to the output log file.
- Can be relative (for example `./logs/app.log`) or absolute.
- Parent directories are created if they do not exist.

### `min_level` (string/enum, optional, default: `info`)
- Minimum severity that will be written to the log file.
- Allowed values: `trace`, `debug`, `info`, `warn`, `error`, `fatal`, `off`.
- Messages below this level are ignored.

### `console_min_level` (string/enum, optional, default: `info`)
- Minimum severity mirrored to console.
- Allowed values: `trace`, `debug`, `info`, `warn`, `error`, `fatal`, `off`.
- This is independent from `min_level`, so file and console can use different thresholds.

### `auto_flush` (boolean, optional, default: `true`)
- `true`: flush each write immediately to disk (safer, slower).
- `false`: keep writes buffered (faster, recent logs may be lost on abrupt crash).

### `append` (boolean, optional, default: `true`)
- `true`: append new logs to existing file.
- `false`: truncate file at initialization and start fresh.

### `daily_rotation` (boolean, optional, default: `true`)
- `true`: write to a date-suffixed file and rotate automatically when day changes.
- `false`: keep writing to the configured `log_file_path` without day-based rotation.
- Filename convention when enabled: `<base_name>_YYYYMMDD<extension>` (example: `app_20260302.log`).

### `config_json_path` (string, optional metadata)
- Stores where configuration was loaded from.
- This value is populated by `load_config_from_json_file(...)` on success.
- Useful for diagnostics and startup logging.

## JSON Configuration Example

```json
{
  "log_file_path": "./logs/app.log",
  "min_level": "debug",
  "console_min_level": "warn",
  "auto_flush": true,
  "append": true,
  "daily_rotation": true
}
```

## Logging Usage

Use level-specific methods directly from the logger instance:

```cpp
auto& logger = optimi::logger::Logger::instance();

logger.info("Service started");
logger.warn("Using fallback configuration");
logger.error("Database connection failed");
logger.fatal("Unrecoverable startup error");
```

These methods automatically capture source file, line, and function for each call site.

## Example App

A multithreaded example application is available at [examples/multi_thread_example.cpp](examples/multi_thread_example.cpp).

Build and run:

```bash
cmake -S . -B build
cmake --build build --target multi_thread_example
./build/multi_thread_example
```

The example spawns multiple worker threads, writes logs concurrently, and verifies the expected line count.

## Third-Party Licenses

External dependency license information is documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).