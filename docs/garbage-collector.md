# Garbage collector

We use strategy *Mark and Swap*, the important places to go are:

### Locals to Mark


|Local to check values| Detail                  |
|---------------------|-------------------------|
| Modules             |  OK                     |
| Native              |  OK                     |
| Global              |  OK                     |
| Local               |  OK                     |
| IP                  |  from the current point |
| Stack               |  OK                     |
| Call Stack          |  OK                     |
