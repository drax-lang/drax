### Pipe operators

Pipe operators are a way to eliminate intermediate values
in a very elegant way.

In drax we have a simple definition of pipe through the syntax |>

We forward the value on the left to the right and this value is forwarded as the first argument of the function on the right.

for example:

```drax
   [1,2,3,4,5] |> list.concat([6])
   # => [1, 2, 3, 4, 5, 6]
```


