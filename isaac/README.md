# ISAAC

Some of the files here are taken without modification from
https://burtleburtle.net/bob/rand/isaacafa.html:

* `randport.c`
* `rand.h`
* `standard.h`
* `randdvec.txt`

The `randport.c` has been copied to `modern.c`, and updated to use
more modern C prototypes, integers, and styles.  Otherwise, the output
is exactly the same.

## Running

You can test ISAAC simply by doing:

```
$ make
```

will:

* build `randport`
* run `./randport > randport.txt`
* check if the output is correct, via `diff randdvec.txt randport.txt`

The build process also verifies the `modern.c` program via the same
process, including a `diff` on its output.

## Seeding

The `modern` program also accepts a seed from its first command-line
argument.  For example:

```
$ ./modern ietf | head -4
29f7dbdb03a9646665a0d78376ed8f67010e93917d8ae79527d30f462a031569
abbb0f5f393cd98bee11d0ff1be9db5c1b6457ea9d6de0c3e71ba7ac9a557ac3
2f97fdb8b1d5446c5ea8ad60d05665f4f206f11dc7b68ff0fe2a1eefa9025dbb
5b407646a7fa4390df71a18510f5eee7daf0cbb35952639534e9c43e3d85dca9
```

## BFD API

The BFD API is documented in the [BFD API](sequence.md) file.
