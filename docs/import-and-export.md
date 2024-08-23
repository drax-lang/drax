# Import and Export

In Drax, you can import libraries or modules from other files using the `import` keyword. These files can then be used within the current scope.

Additionally, the `export` keyword allows you to share specific variables or functions from a module, making them accessible to other files.

## Importing Libraries

### Syntax
To import a library or module from a specific path, use the following syntax:

```drax
import 'path/lib1.dx' as lib
```

- **'path/lib1.dx'**: The relative or absolute path to the module.
- **lib**: The alias you assign to the imported module, the name given to the alias will be the name under which the lib will be accessible in the global scope.

### Example

In this example, we are importing a module from `'path/lib1.dx'` and assigning it the alias `lib`:

```drax
import 'path/lib1.dx' as lib

result = lib.calc(5, 10)
print(result)  // Output will be the result of calc_two(5, 10)
```

In this case, we assume that the module `lib1.dx` contains an exported function called `calc` that performs some calculation.

## Exporting Functions and Variables

### Syntax
To export functions or variables from a file, use the `export` keyword with the following syntax:

```drax
export {
    function_alias: &function_name/arity,
    variable_alias: variable_name
}
```

- **function_alias**: The name to be assigned to the function when exported.
- **&function_name/arity**: The function reference with its arity (number of arguments).
- **variable_alias**: The name to be assigned to the exported variable.
- **variable_name**: The name of the variable inside the module.

### Example

In the file `lib1.dx`, we have the following code:

```drax
name = "drax language"

fun calc_two(a, b) do
    a * b
end

export {
    calc: &calc_two/2,
    name: name
}
```

- We define a function `calc_two` that takes two parameters and returns their product.
- The variable `name` is set to `"drax language"`.
- The `export` block exports the `calc_two` function as `calc` and the `name` variable.

## Example of Usage

Given that we have the following file structure:

- `main.dx`
- `lib1.dx`

### `lib1.dx`
```drax
name = "drax language"

fun calc_two(a, b) do
    a * b
end

export {
    calc: &calc_two/2,
    name: name
}
```

### `main.dx`
```drax
import 'lib1.dx' as lib

result = lib.calc(3, 7)
print(result)  // Output: 21

print(lib.name)  // Output: drax language
```

to run, simply run: `drax main.dx`

In this example:
- The function `calc_two` from `lib1.dx` is called via the alias `lib.calc`, and it multiplies the numbers 3 and 7.
- The variable `name` is also accessed from the `lib` alias, printing the value `"drax language"`.
