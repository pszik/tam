# tam

An emulator for the Triangle Abstract Machine.[^1] The executable
expects one mandatory argument, the name of the TAM program to
execute. Optionally, this can be preceded by the `-t` or `--trace`
option to print each instruction as it is executed.

```shell
$ tam gcd.tam
...output of gcd.tam

$ tam --trace gcd.tam
...output of gcd.tam with each instruction printed out
```

[^1]:
    D.A. Watt and D.F. Brown, _Programming Language Processors in Java:
    Compilers and Interpreters_. Harlow, Essex: Prentice Hall, 2000.
