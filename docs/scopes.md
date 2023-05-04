# Scopes

Local scopes can only read variables from the global scope, no longer have permission to change them.

Ex:

```drax
    global = "abc"

    fun change() do
        print(global)  # => "abc"
        global = "cba"
        print(global)  # => "cba"
    end

    change()    
    print(global)  # => "abc"

```

When you assign a new value to a global variable within a function's scope, a new variable is declared with the same name as the global variable and it takes precedence.
