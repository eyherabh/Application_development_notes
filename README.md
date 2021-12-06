# Application development notes

## Compiling PLINKv2.0

PLINK is an open-source program for whole-genome association analysis, among other things. Several versions exist: v0.99 to v1.07 (http://zzz.bwh.harvard.edu/plink/), v1.9 (https://www.cog-genomics.org/plink/), and v2.0 (https://www.cog-genomics.org/plink/2.0/). Both v1.9 and v2.0 are still under development (beta and alpha states, respectively) but can be either more capable and more performant than the stable versions.

Although the distributed PLINK binaries may well be sufficient in most cases, compiling from source may allow to support other platforms, extract more performance or experiment with its source code, among other things. However, when compiling PLINKv2.0, the issue reported [in this page](https://github.com/chrchang/plink-ng/issues/152) may arise. The issue can be solved by exporting the environment variable `CPATH` as in the snippet below. Other issues may arise with missing libraries that can be solved in a similar manner.


In my Fedora 33, I managed to compiled PLINK v2.0 as follows
```
# Starting a subshell.
(
    # Exit the subshell on error.
    set -e
    
    # Building directory for the PLINKv2 source. Remove if is already exists.
    mkdir plinkv2 
    
    # Change to the PLINKv2 directory. Modify as necessary if not created by this snippet.
    cd plinkv2 
        
    # Retrieving only the source for PLINKv2.0.
    git init
    git remote add origin https://github.com/chrchang/plink-ng.git
    git sparse-checkout set 2.0
    git pull origin master

    # Change to the location of the PLINKv2 source code within the repository.
    cd 2.0 
    
    # Adds packages and headers that were missing in my system.
    dnf install blas blas-devel lapack lapack-devel

    # Exporting CPATH with values for finding the C headers of zstd and cblas.
    export CPATH=$PWD/zstd/lib:/usr/include/cblas
    
    # Building with as many processes as the number of processors available for this shell.
    # Set to a number if nproc is not available.
    ./build.sh -j $(nproc) 
)
```


## Sign files and packages with gpg

GPG can be generated with the command 
```
gpg --gen-key
```
which will ask for name and email, among other details, and may open a new window for entering the password and confirming it. The window opening can be avoided by using
```
gpg --gen-key --pinentry-mode loopback

```
which will ask for the password in the terminal, albeit only once.


## GLIB hash tables

The GNOME library [GLIB](https://developer.gnome.org/glib/) provides utilities for building hash tables. However, some implementation and usage details may cause errors and corrupt data in ways that are not self-evident. My analysis is based on [the source code](https://gitlab.gnome.org/GNOME/glib) (git commit 46c34ea20fc020cde5d07c69747a5d4d12197e96) and the [GHashTable documentation](https://developer.gnome.org/glib/stable/glib-Hash-Tables.html). The sections below present my findings with the aim of easing library usage, avoiding common drawbacks and improving software safety and accuracy.

### Do not mix sets and hash tables

The [GHashTable documentation](https://developer.gnome.org/glib/stable/glib-Hash-Tables.html) states at the end of the `Description` section that sets can be efficiently implemented by passing key-value pairs where `key==value`. Since the functions `g_hash_table_insert`, `g_hash_table_replace` and `g_hash_table_add` take pointers, that means the the key and the value must point to the same object. This implementation is there deemed efficient because no memory is allocated to store the values, but what are the values: the pointers or the objects pointed to? Discussing the causes, be them misinterpretation, poor writing skills, etc. is out of scope. Instead, I will focus on documenting the memory management within the hash table. 

The functions mentioned above take as `key` and `value` pointers to objects. Memory allocation is never performed for these objects, but for arrays containing these pointers. Only one array is allocated while `key==value`. However, on the first occurrence of `key!=value`, a second array is built. This process occurs within `g_hash_table_ensure_keyval_fits`. **The change is irreversible**, that is, once a `key!=value` is added, removing the pair will not remove the array previously generated so that the hash table is again treated as a set.

### Pass `NULL` for `value_destroy_func` when using sets

The function `g_hash_table_new_full` allows one to pass functions `key_destroy_func` and `value_destroy_func`. As their name indicated, these functions destroy the objects pointed to by the key and the value pointers. These functions are called in succession within `g_hash_table_remove_all_nodes` and `g_hash_table_insert_node` (which is called by `g_hash_table_insert`, `g_hash_table_replace` and `g_hash_table_add`) even if both key and value point to the same object. This is exactly the case of sets, but could also occur for hash tables. In those cases, an error will most likely occur at runtime because of attempting to destroy the same object twice. 

The error could have been avoided should key and value have been set to `NULL` after destruction, at least when `key_destroy_func` and `value_destroy_func` operate as `free` (.i.e. perform no actions on `NULL` pointers). Instead, in the case of sets, the issues can be avoided by supplying only a function for `key_destroy_func` and settting `value_destroy_func` to NULL. In the case of hash tables, the issue can only be avoided by making sure that for no key-value pair  the key and the value point to the same object.

The solutions above do not prevent the double-destruction runtime error from happening when some keys and values in different key-value pairs point to the same object. If objects cannot be copied to ensure that the situation does not occur, then object destruction will need to be handled without the aid of `key_destroy_func` and `value_destroy_func`.


## The lost art of structure packing, revisited

A few observations on the article [The lost art of structure packing](http://www.catb.org/esr/structure-packing/) basec on the [C17 draft](https://web.archive.org/web/20181230041359if_/http://www.open-std.org/jtc1/sc22/wg14/www/abq/c17_updated_proposed_fdis.pdf).

+ Section 4 says that static variables are not required to be allocated in source order by the C standard.
+ Section 6 says that padding bits are not required to be zeroed by the C99 standard. The C17 draft says:
++ In 6.7.9-10, that for structures with static and thread storage duration and no initializer, the padding bits are zeroed.
++ In 7.24.4.1 footnote 315 and J.1, that the content of padding used for alignment within structures is indeterminate.
+ Section 7 says that C does not reorder fields. This is mentioned in the C17 draft 6.7.2.1-15.


## Capitalization convention in python argparse, revisited

Format and style consistency are paramount when writing the help manuals of command-line tools. Their lack may hinder productivity for users, for colleges and for one's future self. Unfortunately, python argparse [1] chose a capitalization convention that differs from other common Linux tools. In additino, as I explain below, I find this choice detrimental to readability and confusing to users. The absence of public interfaces for format customization [2] undermines the integration of new python tools into existing frameworks that follow different conventions. In this section, I will first discuss the peculiar choice of capitalization convention followed by argparse, and show how to customize it, albeit using its private interface.

### How argparse help capitalization differs from others

Consider some python-based command-line tool, here called `argparse_cap_test.py`, that starts with the following code
```
#!/usr/bin/python3
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Capitalization test')
    for opt in ['--get', '--insert', '--delete']:
        parser.add_argument(opt, action = 'store_true', help = f'{opt[2:].capitalize()} a key-value pair')
    args = parser.parse_args()
```
Calling it as `./argparse_cap_test.py -h` produces the following
```
usage: arg_parse_cap_test.py [-h] [--get] [--insert] [--delete]

Capitalization test

optional arguments:
  -h, --help  show this help message and exit
  --get       Get a key-value pair
  --insert    Insert a key-value pair
  --delete    Delete a key-value pair
```
Note that `usage` (first line) and `optional` (fifth line) appear uncapizalized. Compare this with the output produced by `pigz -h`
```
Usage: pigz [options] [files ...]
  will compress files in place, adding the suffix '.gz'. If no files are
  specified, stdin will be compressed to stdout. pigz does what gzip does,
  but spreads the work over multiple processors and cores when compressing.

Options:
  -0 to -9, -11        Compression level (level 11, zopfli, is much slower)
  --fast, --best       Compression levels 1 and 9 respectively
  -b, --blocksize mmm  Set compression block size to mmmK (default 128K)
  -c, --stdout         Write all processed output to stdout (won't delete)
```
Contrary to argparse, here `Usage` (first line)  and `Options` (sixth line) appear capitalized. 

The convention chosen by argparse was said in [3] to be not only an argparse convention but, most importantly, a convention of English grammar. The fact that many Linux tools do not follow this convention was attributed to the fact that the developers of these tools come from “a wide range of cultural backgrounds with varying levels of mastery of English”. However, in neither case were authoritative references or further justification provided.

The hypothesis that varying cultural backgrounds and levels of English mastery are the culprit for the variability in capitalization may look sound at first glance. Indeed, both affect native and non-native speakers alike in many languages. However, it falls short in at least two different aspects. First, writing help manuals in command-line tools is oftentimes an afterthought, if not regarded as a burden, for developers and managers alike. Hence, the variability may be partially explained by the variation in the amount of dedicated resources. Second, it overlooks the fact that multiple style guides exist in English, which do not always coincide in their advice. The latter point above also cast doubts about the claim that argparse convention is an English convention (a discussion of English conventions is beyond the scope of this article). 


### Why is capitalization important?

I believe that capitalization is important for communication and productivity. Without them, users are left with the impression that the uncapitalized fragment is part of a sentence, thereby causing further delays and distractions. Having said that, note that the webpages of the Longman English Dictionary [4] and Cambridge Dictionary [5]  contain plenty of uncapitalized fragments, without resulting in the confusion I just mentioned. This lack of confusion can be explained by the fact that, in most if not all those cases, the uncapitalized fragments constitute items in order or unordered lists clearly delimited by bullets and other symbols.


### Customize argparse help formatter

As mentioned in [3], the argparse title for positional and optional arguments can be customized through the private properties `_positionals.title` and `_optionals.title` of the `argparse.ArgumentParser` class. Capitalizing `usage` can be done, for example, by overloading either the method `add_usage` or the method `_format_usage` of the `argparse.HelpFormatter` class and assigning the overloaded class to `argparse.formatter_class`. The former is simpler than the latter, and it is shown below.
```
import argparse

class CapFormatter(argparse.HelpFormatter):
    def add_usage(self, usage, actions, groups, prefix="Usage: "):
        if usage is not argparse.SUPPRESS:
            args = usage, actions, groups, prefix
            self._add_item(self._format_usage, args)
        
parser = argparse.ArgumentParser()
parser.formatter_class = CapFormatter

parser.parse_args(['-h'])
```

### References

[1]: https://docs.python.org/3/library/argparse.html
[2]: https://bugs.python.org/issue42966
[3]: https://stackoverflow.com/questions/35847084/customize-argparse-help-message
[4]: https://www.ldoceonline.com/dictionary/complete
[5]: https://dictionary.cambridge.org/dictionary/english/complete

1. [Argparse documentation](https://docs.python.org/3/library/argparse.html)
2. [Request for argparse customizable help formatter](https://bugs.python.org/issue42966)
3. [Stackoverflow: Customize argparse help message](https://stackoverflow.com/questions/35847084/customize-argparse-help-message)
4. [Longman English Dictionary](https://www.ldoceonline.com/dictionary/complete)
5. [Cambridge Dictionary](https://dictionary.cambridge.org/dictionary/english/complete)
