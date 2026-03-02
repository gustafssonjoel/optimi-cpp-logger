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
  "auto_flush": true,
  "append": true,
  "daily_rotation": true
}
```