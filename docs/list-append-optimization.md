# List append optimization

Goal: make `List.concat(acc, [x])` and `acc ++ [x]` (building a list one
element at a time in a recursive loop) cost O(1) amortized instead of O(n),
without breaking immutability. Both go through `dlist_concat` in
`src/dstructs.c`.

Code used in all examples:

```elixir
a = [1, 2, 3]
b = List.concat(a, [4])
c = List.concat(a, [9])
```

---

## 1. Before: always copy

Every `concat` allocates a new buffer and copies everything.

```
a = [1,2,3]
    buffer A: [1][2][3]

b = concat(a, [4])          copy 3 elements + 1 new
    buffer B: [1][2][3][4]

c = concat(a, [9])          copy 3 elements + 1 new
    buffer C: [1][2][3][9]
```

Building a list of n elements in a loop:

```
step 1:  copy 0   -> [1]
step 2:  copy 1   -> [1,2]
step 3:  copy 2   -> [1,2,3]
...
step n:  copy n-1
total = 0+1+2+...+(n-1) = n(n-1)/2   ->  O(n^2)
```

20 000 elements = ~200 million element copies (0.245 s).

---

## 2. After: share the buffer and write in the free space

### The pieces

A list is a small **header** pointing into a **buffer**. Several headers can
point to the same buffer (a *view*). The header that owns the buffer keeps
`used`: how many slots were ever written.

```
header a                     buffer (cap = 8)
+-------------+              slot: 0   1   2   3   4   5   6   7
| elems  ─────┼────────────►      [1] [2] [3] [ ] [ ] [ ] [ ] [ ]
| length = 3  |                    └──── a ────┘
| owner  = -  |   used = 3   (the slots that were written)
+-------------+
```

### Rule

> Write in place **only if** the end of the list is exactly where the written
> area ends (`elems + length == buffer + used`) and there is room.

### Step by step

```
b = List.concat(a, [4])

  end of a = slot 3, used = 3  -> equal, there is room -> WRITE IN PLACE

  slot:  0   1   2   3   4   5   6   7
        [1] [2] [3] [4] [ ] [ ] [ ] [ ]
         └─── a ───┘
         └──── b ────┘                  used = 4

  a is untouched: length is still 3, slots 0..2 never change.
  b is a NEW header (length 4) over the same buffer.
  Cost: write 1 element. No copy.
```

```
c = List.concat(a, [9])

  end of a = slot 3, used = 4  -> NOT equal (slot 3 already belongs to b)
  -> cannot write there, it would overwrite b's 4. COPY.

  buffer 1:  [1] [2] [3] [4] [ ] ...      a and b
  buffer 2:  [1] [2] [3] [9] [ ] ...      c (new buffer, with spare room)
```

This is why `a`, `b` and `c` are all still correct: the slot is written by
the first one that asks, and everybody else copies.

### Special cases

- `xs ++ []` returns `xs` itself and `[] ++ ys` returns `ys`: lists are
  immutable, so there is nothing to copy.
- If the result would have more than `INT_MAX` elements, `dlist_concat`
  returns `NULL` and the caller raises an error (`Concat result is too large`).

### The loop case (the one that matters)

```
acc = []        concat(acc,[1])   concat(acc,[2])   concat(acc,[3])   ...
                 write slot 0      write slot 1      write slot 2

at each step the previous list is the "last" one, so its end == used
-> in-place write. When the buffer is full: copy once into a bigger one
(cap + 50%).
```

```
total cost = n writes + a few copies (geometric growth) = O(n) amortized
```

| n | before | after |
|---|---|---|
| 20 000 | 0.245 s | 0.017 s |
| 300 000 | quadratic | 0.3 s |

---

## 3. Why immutability still holds

```
        view a (len 3)         view b (len 4)
        ┌──────────┐          ┌─────────────┐
buffer: [1] [2] [3] [4] [ ]
                     ▲
                     └ only b can see this slot; a never reads past 3
```

1. Nobody ever *rewrites* a slot that someone can see.
2. `used` only grows, so a slot is claimed once: the second writer sees
   `end != used` and copies.
3. A list's `length` never changes after creation.

### Invariant for contributors

A buffer that is shared must never be reallocated or have its existing slots
written. Calling `put_value_dlist` or `realloc` on `elems` is only valid for a
list that was just created and has not been returned to Drax code yet. All
current call sites follow this (lists built by literals, `zip`, `map`,
`filter`, string splitting, and `api_list_push`, which is guarded by
`made_has`).

---

## 4. Disadvantages

| # | Disadvantage | Detail |
|---|---|---|
| 1 | **Memory held longer** | A tiny view keeps the whole buffer alive. `List.tail(big)` of 1 element retains the full buffer until that view dies. |
| 2 | **Wasted spare room** | After a copy, 50% + 4 extra slots are reserved. A list that is never appended to again wastes up to ~33% of its buffer. Only applied when both sides are non-empty and the right side has <= 8 elements. |
| 3 | **Fragile invariant** | Any code that calls `realloc` or `put_value_dlist` on an already shared list would leave other views with dangling pointers. Nothing enforces this today. |
| 4 | **Only the lone "last" list is fast** | Appending to an old version (`c` above) copies. Patterns that branch repeatedly from one base get no gain (and pay the 50% slack). |
| 5 | **Prepend is not helped** | `List.concat([x], acc)` still copies everything. |
| 6 | **Not thread safe** | The check (`end == used`) and the write are not atomic. If lists are shared between threads, two threads could both pass the check. Not verified in Drax. |
| 7 | **GC bookkeeping** | Values written past a parent's `length` are only marked by the views that see them. Correct today, but one more thing to keep in mind when changing the GC (see [garbage-collector.md](./garbage-collector.md)). |

