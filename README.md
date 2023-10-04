# Drax 
![https://github.com/jeantux/drax/actions/workflows/drax-build.yml/badge.svg](https://github.com/jeantux/drax/actions/workflows/drax-build.yml/badge.svg)
**Dynamic and Rapid Abstract eXecutor**

Drax is a lightweight project, dynamically typed, functional programming language with a focus on integrations. <br/>

### Documentation
drax documentation is available in [docs](./docs/index.md)

### Contributing

Run initial environment settings with:

```
make config
```

To compile run:

```
make
```

if your distribution doesn't support libedit you can compile the project without it using
```
make LIGHT=1
```

After compiling it will generate the final binary in the `/bin` directory, to be executed by Makefile execute:
```
make run
```

To compile in debug mode run:

```
make debug
```


