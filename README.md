# Tips_and_tricks

## Compiling PLINKv2.0
In Fedora 33

```
set -e
mkdir plinkv2
cd plinkv2
git init
git remote add origin https://github.com/chrchang/plink-ng.git
git sparse-checkout set 2.0
git pull origin master
cd 2.0
dnf install blas blas-devel lapack lapack-devel
export CPATH=$PWD/zstd/lib:/usr/include/cblas
./build.sh -j $(nproc)
```
