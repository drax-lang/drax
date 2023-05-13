# Functions

Functions in drax can be defined as follows:

```drax
fun get_name() do
   "drax"
end
```

the last expression of the function will be its return, in the example above it returns the string "drax".

It is possible to have functions with the same name as long as they have different numbers of arguments, let's know which function to execute according to the number of arguments passed in the call.

```drax
fun sum(a) do a end

fun sum(a, b) do a + b end

sum(1)       # => 1
sum(1, 2)    # => 3
sum(1,2,3)   # => error: function 'sum/3' is not defined
```

If you want to get a function definition to manipulate, you can do it using the `&` operator.

```drax
myfun = &sum/2
```

note that it is necessary to inform how many arguments my function has, as there may be several definitions with the same name

### lambdas

lambdas are functions that can be treated as values, you can receive functions as arguments of functions or store them easily in variables.
are considered anonymous functions, as they do not have names.

Here is an example of use:

```drax
myfun = lambda (a, b) a + b end
myfun()
```

Functions always respect [scope rules](./scopes.md).
