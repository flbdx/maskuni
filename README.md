# maskgen

A standalone fast word generator in the spirit of [hashcat's mask generator](https://hashcat.net/wiki/doku.php?id=mask_attack) with unicode support .

## Overview

Maskgen is a standalone word generator based on templates (masks) which describe the charset for each position of the word.

Maskgen is largely compatible with the syntax of [Hashcat](https://hashcat.net/hashcat/) and [Maskprocessor](https://github.com/hashcat/maskprocessor). In addition, Maskgen supports :
- Unicode charsets
- List of masks (supported by Hashcat but not by Maskprocessor)
- A few more built-in charsets and less restrictions on user-defined charsets
- Control of the word delimiter
- A syntax for splitting the generation in equal parts (jobs)

The unicode support implies that all inputs must be valid UTF-8 and the output is UTF-8 encoded.

## Building

Maskgen requires:
- GCC or Clang (GCC likely to produce faster code)
- CMake
- POSIX environment
- Should build at least on Linux, Cygwin and Msys2/mingw64

Building with cmake:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

## Mask based word generation

If you are not familiar with mask-based word generation I would suggest reading the [dedicated page on Hashcat's wiki](https://hashcat.net/wiki/doku.php?id=mask_attack).

Masks are templates which define which character are allowed at each position of a word. For each position, a mask defines either :
- a static character
- a set of allowed characters (charset)

The word generator will generate all possibilities matching the mask.

For example, if a mask defines `<digit>@passwd<digit><digit>`, the word processor will generate 1000 words ranging from `0@passwd00` to `9@passwd99`.

## Charsets

A charset is a named variable describing a set of characters.

In its default mode, Maskgen allows only characters encoding on a fixed with 8-bit encoding (same as Hashcat). As an extension, Maskgen also supports full unicode charsets.

Charsets are named by a single character. They are referred by the syntax `?K` where `K` is the name of the charset.

*When defining a charset, you may include another charset by writing its name (`?K`). A single `?` must be escaped by writing `??`.*

Maskgen defines the following builtin charsets :

```
?l = abcdefghijklmnopqrstuvwxyz
?u = ABCDEFGHIJKLMNOPQRSTUVWXYZ
?d = 0123456789
?s =  !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
?a = ?l?u?d?s
?h = 0123456789abcdef
?H = 0123456789ABCDEF
?n = \n (new line)
?r = \r (carriage ret)
?b = 0x00 - 0xFF (only without --unicode)
```

The last charset `?b` is not defined when using the unicode mode as the upper values can't be encoded in UTF-8.

## Masks

Masks are single line strings build by concatenating what to use at each position :
- a single char, which will be always used at this position
- a named charset (`?K`)

A single `?` must be escaped by writing `??`.

For example the previous example `<digit>@passwd<digit><digit>` would be written `?d@passwd?d?d`.

Maskgen iterates from right to left. So the output of this mask is :
```
0@passwd00
0@passwd01
0@passwd02
0@passwd03
0@passwd04
0@passwd05
0@passwd06
0@passwd07
0@passwd08
0@passwd09
0@passwd10
0@passwd11
[...]
9@passwd94
9@passwd95
9@passwd96
9@passwd97
9@passwd98
9@passwd99
```

## Mask lists

Mask lists are files containing one or more masks definitions (one per line). The syntax for mask lists is *almost* the same as Hashcat's own syntax. The masks are iterated sequentially.

Mask lists can embed custom charset definitions for each mask.

The general syntax for a single line is :
`[:1:,][:2:,]...[:9:,]:mask:`
where the placeholders are as follows:
- `:1:` the named custom charset '1' (overrides `--custom-charset1` or `--charset`) will be set to this value, optional
- `:2:` the named custom charset '2' (overrides `--custom-charset2` or `--charset`) will be set to this value, optional
- ...
- `:9:` the named custom charset '9' (overrides `--charset`) will be set to this value, optional
- `:mask:` the mask which may refer to the previously defined charsets

As the comma `,` is used to delimit the custom charsets and the mask, it's necessary to escape a `,` in a charset or a mask definition by writing `?,`.

There are a few differences with Hashcat's syntax:
- `,` are escaped with `?,` instead of `\,` (as a consequence you can't use a charset named `,` in a mask file if you ever feel the urge to name a charset `,`)
- comments are not supported
- Maskgen allows up to 9 custom charsets at the beginning of a line

## Usage

```
Usage: maskgen [OPTIONS] (mask|maskfile)
Generate words based on templates (masks) describing each position's charset

 Behavior:
  -u, --unicode                Allow UTF-8 characters in the charsets
                               Without this option, the charsets can only
                               contain 8-bit (ASCII compatible) values
                               This option slows down the generation and
                               disables the '?b' built-in charset
 Range:
  -j, --job=J/N                Divide the generation in N equal parts and
                               generate the 'J'th part (counting from 1)
  -b, --begin=N                Start the generation at the Nth word
                               counting from 0
  -e, --end=N                  Stop after the Nth word counting from 0

 Output control:
  -o, --output=FILE            Write the words into FILE
  -z, --zero                   Use the null character as a word delimiter
                               instead of the newline character
  -n, --no-delim               Don't use a word delimiter
  -s, --size                   Show the number of words that will be
                               generated and exit
  -h, --help                   Show this help message and exit

 Charsets:
  A charset is a named variable describing a list of characters. Unless
  the --unicode option is used, only 8-bit characters are allowed.
  The name of a charset is a single character. It is reffered using '?'
  followed by its name (example : ?d). A charset definition can refer to
  other named charsets.

  Built-in charsets:
   ?l = abcdefghijklmnopqrstuvwxyz
   ?u = ABCDEFGHIJKLMNOPQRSTUVWXYZ
   ?d = 0123456789
   ?s =  !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
   ?a = ?l?u?d?s
   ?h = 0123456789abcdef
   ?H = 0123456789ABCDEF
   ?n = \n (new line)
   ?r = \r (carriage ret)
   ?b = 0x00 - 0xFF (only without --unicode)

  Custom charsets:
   Custom named charsets are defined either inline or by reading a file.
   To include a single '?' in a charset, escape it with another '?' (??).
   Pay attention to trailing newline when reading a file or to the shell
   expansion for inline definitions ('?' or '*' chars...)

   -1, --custom-charset1=CS    Define the charsets named '1', '2', '3' or
   -2, --custom-charset2=CS    '4'. The argument is either the content of
   -3, --custom-charset3=CS    the charset or a file to read.
   -4, --custom-charset4=CS

   -c, --charset=K:CS          Define a charset named 'K' with the content
                               'CS'. 'K' may be an UTF-8 char onlye if
                               --unicode is used. Otherwise it's a single
                               8-bit char.

 Masks:
  Masks are templates defining which characters are allowed for each
  positions. Masks are single line strings build by concatenating
  for each positions either: 
  - a static character
  - a charset reference indicated by a '?' followed by the charset
    name
  For example, '@?u?l?l?l?d@' would generate the words from
  '@Aaaa0@' to '@Zzzz9@'

  The mask argument is either a single mask definition or a file
  containing a list of masks definitions.

  Mask files can also embed charset definitions. The general syntax for
  a single line is:
   [:1:,][:2:,]...[:9:,]:mask:
  where the placeholders are as follows:
   :1: the named custom charset '1' (overrides --custom-charset1 or 
        --charset) will be set to this value, optional
   :2: the named custom charset '2' (overrides --custom-charset2 or 
       --charset) will be set to this value, optional
   ...
   :9: the named custom charset '9' (overrides --charset) will be set to
       this value, optional
   :mask: the mask which may refer to the previously defined charsets

  The characters ',' and '?' can be escaped by writing '?,' or '??'.
```

## Examples

### Defining and using custom charsets

A custom charset can be defined inline as an argument :
```
$ ./maskgen -1 '01' '?1?1?1'
000
001
010
011
100
101
110
111
```

Or by writing it inside a file (pay attention to any unwanted end of line character):
```
$ echo -n 01 > charset_binary
$ ./maskgen -1 charset_binary '?1?1'
00
01
10
11
```

The options `-1`, `-2` to `-4` (or the long forms `--custom-charset1` to `--custom-charset4`) are shortcuts to define the charsets `?1` to `?4`.

Maskgen will first try to open and read a file with the given value. If none is found then the charset is defined from the argument.

The option `-c` (or `--charset`) allows to define a charset with an arbitrary name :
```
$ ./maskgen -c 'L:?l?u' '?L'
a
b
[...]
Z
Y
Z
```

Masken will fully expand the user defined charsets (replace any `?X` by the corresponding charset) and uniquify the charsets.

### Single masks

A single mask can be given inline or in a mask list file.

```
$ ./maskgen --size ?d?d?d?d?l?l
6760000
$ echo ?d?d?d?d?l?l > mask
$ ./maskgen --size mask
6760000
```

Note that commas `,` need to be escaped with `?,` when used inside a mask file.

### List of masks

Mask lists are a simple way to generate words over multiple patterns in a single invocation.
For example :
```
$ cat masks_nums_1_10 
?d
?d?d
?d?d?d
?d?d?d?d
?d?d?d?d?d
?d?d?d?d?d?d
?d?d?d?d?d?d?d
?d?d?d?d?d?d?d?d
?d?d?d?d?d?d?d?d?d
?d?d?d?d?d?d?d?d?d?d
$ ./maskgen --size masks_nums_1_10 
11111111110
```

Example with inline charsets definition :

```
$ cat masks
?l?u,?1?l?l?l?l
?l?u,*$,?1?l?l?l?l?2
$ ./maskgen ./masks|grep -ie 'abcde'
abcde
Abcde
abcde*
abcde$
Abcde*
Abcde$
```

### Unicode charsets

The default mode of Maskgen is limited to 8-bit characters, which may use any character encoding. Therefore it's not possible to use characters that are usually only available on multibyte character encodings such as UTF-8.

For example, it's not possible to use smileys with the default mode of Maskgen (same with Hashcat or maskprocessor):
```
$ ./maskgen -1 '😂❤😍😊😭😘👍' emoji?1
emoji�
emoji�
emoji�
emoji�
[...]
```

The different bytes of this charset are read byte by byte and each byte is added to an 8-bit charset.

Maskgen can handle such character with its unicode support, enabled with `-u` or `--unicode`. *When enabled, all inputs (charsets and masks) are treated as UTF-8 encoded and the output is UTF-8 encoded*.

```
$ ./maskgen -u -1 '😂❤😍😊😭😘👍' emoji?1
emoji😂
emoji❤
emoji😍
emoji😊
emoji😭
emoji😘
emoji👍
```

When unicode support is enabled, Maskgen is significantly slower as it will iterate over 32-bits unicode codepoints instead of 8-bits characters and therefore read or write 4 times as much memory for the same word width. And of course Maskgen must encode its output in UTF-8.

Note that there are many subtleties when it comes to unicode characters. Some characters are formed by combining several characters. Emoji characters are complex and masks are not the best approach for emoji generation (see [Unicode TR#51](https://unicode.org/reports/tr51/)). In the previous example, the heart ❤︎ (U+2764) is actually not the very common red heart emoji ❤️ which is formed by combining U+2764 with the variant selector U+FE0F. As another example, "👨‍👩‍👦‍👦" should be represented as a single character (if supported by the browser and if it survived the markdown rendering) but is actually a sequence of 7 characters.

### Partitioning the generation

To split the word space in several part, Maskgen supports two mechanisms :
- explicit numbers of the first and last words
- spliting in N equal parts (jobs)

The first method use the options `-b` (`--begin`) and/or `-e` (`--end`) with words numbered from 0:
```
$ ./maskgen -b 5 -e 7 ?d
5
6
7
```

The second method defines a job number N out of a given total. Jobs are numbered from 1.

```
$ ./maskgen -j 1/5 ?d
0
1
$ ./maskgen -j 4/5 ?d
6
7
```

The job specification is convenient to efficiently run treatments in parallel. For example using GNU parallel:
```
$ work() { ./maskgen -j "$1/$2" ?l?l?l?l?l?l | mytool; }
$ export -f work
$ N_JOBS=500
$ seq $N_JOBS | parallel -j 8 --progress work {} $N_JOBS
```
