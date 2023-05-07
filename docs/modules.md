# Modules

Drax offers some native modules

All basic functions that are not present in the native types are probably included in the modules.

### os (operating system)
This module aims to facilitate communication with the operating system.

**command**: The command function takes the command in string format as an argument and returns a string with the result.

*if your compiler supports popen, the command will use it.*
*Otherwise, and on unix systems, we will use /bin/sh.*

Example:
```drax
  > os.command("ls /")
  "bin\nboot\ndev\ntmp\nusr\nvar\n"
```

**get_env**: The get_env function retrieves the value of the environment variable specified by the given name.


Example:
```drax
  > os.get_env("HOME")
  "/home/drax"
```