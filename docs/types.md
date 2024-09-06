# Types
Drax is a dynamically typed language, which means that the type of the variable is determined by the content present in it.

The number, string, list and frame types have built-in functions to facilitate development.

Follows the natural types of language.

---

### Nil
This value is the representation of empty/null.

---

### Boolean
Are reserved for *true* and *false* values

---

### Numbers
Both integers and floating point numbers are internally represented as double.

Number can be created with some prefixes and have different interpretations like:

    0b: represents a number in binary
    0o: represents a number in octal
    0x: represents a number in hexadecimal

Here are some accepted notations:

     123
     123.4
     123.456
     0x7b
     0b1111011
     0o173
---

### String
String is represented internally as a sequence of characters.

you can access and combine strings 

```drax
  > "foo" ++ "bar"
  "foobar"
  > "foo"[0]
  "f"
```

or manipulate these characters with some native functions:

**split**: returns a list of strings split by the delimiter,

Examples: 
```drax
  > String.split("foo\nbar", "\n")
  ["foo", "bar"]
```

**length**: returns the length of the string.

Examples: 
```drax
  > String.length("drax")
  4
```

**to_number**: convert target string to a number.

Examples: 
```drax
  > String.to_number("123")
  123
```

**copy**: Is a function that returns a substring within a string.

This function has two arguments:
  1 - The first is which position to start copying
  2 - The second is the number of characters to be returned.

 If the second argument is omitted, then the value 1 is assumed
 If the second argument is a negative value, the count is made from the end to the beginning

Examples: 
```drax
  > String.copy("drax lang", 0)
  "d"

  > String.copy("drax lang", 0, 4)
  "drax"

  > String.copy("drax lang", -4, 4)
  "lang"

  > String.copy("drax lang", 0, 0)
  ""
```   
**get**: this function returns the character of the index that is passed as an argument.
however it is much more common to use `[` ` ]` which internally maps to the `get` function
```drax
  > String.get("drax lang", 0)
  "d"

  > "drax lang"[0]
  "d"
```  

---

### List
Lists in drax is a sequence of elements that can be accessed by indexes.

This structure is not limited to a single data type, for example:

```drax
  ["foo", 123, nil, true, { name: "drax" }, ["sublist"]]
```

**length**: returns the length of the List.

Examples: 
```drax
  > ["foo", "bar"].length
  2
```

**get**: this function returns the element of the index that is passed as an argument.
however it is much more common to use `[` ` ]` which internally maps to the `get` function
```drax
  > ["drax", "lang"].get(0)
  "drax"

  > ["drax", "lang"][0]
  "drax"
```  

---

### Frame
frame is the main structure that stores data like key and value.

`.` is used to fetch the elements of that structure, for example:


```drax
  > f = {lang: "drax"}
  > f.lang
  "drax"
```  
