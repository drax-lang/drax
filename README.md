# Drax 
![https://github.com/jeantux/drax/actions/workflows/drax-build.yml/badge.svg](https://github.com/jeantux/drax/actions/workflows/drax-build.yml/badge.svg)

<span style="color: orange">We encourage developers to explore and provide feedback, but be aware that the language may not yet be stable for production use.
Stay tuned for updates and future releases!</span>

**Dynamic and Rapid Abstract eXecutor**

Drax is a lightweight project, dynamically typed, functional programming language with a focus on integrations. <br/>


### Learning

Know the language in [Documentation](https://drax-lang.org/documentation/)

### Contributing

drax documentation is available in [docs](./docs/index.md)

Run initial environment settings with:

```
make config
```

To compile run:

```
make all
```

if your distribution doesn't support libedit you can compile the project without it using
```
make all LIGHT=1
```

After compiling it will generate the final binary in the `/bin` directory, to be executed by Makefile execute:
```
make run
```

To compile in debug mode run:

```
make debug
```


