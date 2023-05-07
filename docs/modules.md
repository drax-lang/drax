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

**mkdir**: try to create a directory for the path passed as an argument

Example:
```drax
  > os.mkdir("folder")
  true
```
You can pass a second argument that indicates the permission of the folder.

the second argument must be an octal number.

Example:
```drax
  > os.mkdir("folder", 0o777)
  true
