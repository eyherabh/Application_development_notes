# Tips_and_tricks

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
