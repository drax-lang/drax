# Modules

Drax offers some native modules

All basic functions that are not present in the native types are probably included in the modules.

### os (operating system)
This module aims to facilitate communication with the operating system.

**cmd**: The command function takes the command in string format as an argument and returns a string with the result.

If the command fails, an error is generated.

*if your compiler supports popen, the command will use it.*
*Otherwise, and on unix systems, we will use /bin/sh.*

Example:
```drax
  > Os.cmd("ls /")
  "bin\nboot\ndev\ntmp\nusr\nvar\n"
```

**cmd_with_status**: The command function takes the command in string format as an argument and returns a list with the status and result.

*if your compiler supports popen, the command will use it.*
*Otherwise, and on unix systems, we will use /bin/sh.*

Example:
```drax
  > Os.cmd_with_status("ls /")
  [0, "bin\nboot\ndev\ntmp\nusr\nvar\n"]
  
  > Os.cmd_with_status("foo_bar")
  [127, ""]
```

**get_env**: The get_env function retrieves the value of the environment variable specified by the given name.

Example:
```drax
  > Os.get_env("HOME")
  "/home/drax"
```

**mkdir**: try to create a directory for the path passed as an argument

Example:
```drax
  > Os.mkdir("folder")
  true
```
You can pass a second argument that indicates the permission of the folder.

the second argument must be an octal number.

Example:
```drax
  > Os.pmkdir("folder", 0o777)
  true
```

### core
the core module offers functions that allow you to manipulate the drax virtual machine

**exit**: terminates the vm process, this function receives the error status as an argument

Example:
```drax
  > Core.exit(1)
```
### frame
All functions that manipulate the frame always return a new element, never changing the source element

**put** put a value in the frame

Example:
```drax
  > f = {name: "drax"}
  > Frame.put(f, "value", 123)
  {name: "drax", value: 123}
  > f
  {name: "drax"}
```


