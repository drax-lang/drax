# Beorn

Beorn is a simple functional language implementation.
The proposal is to implement a functional language written in c.
### Examples

```beorn
fun say [text] {(text)}
(say "Hello World")
```

### Contributing

Must be installed libedit

Arch Linux
```
sudo pacman -S libedit
```

Run initial environment settings with:

```
make config
```

To compile in debug mode:

```
make debug
```


