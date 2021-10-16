# efa-libfabric-bench
Benchmarking tool for AWS EC2's Elastic Fabric Adapter(EFA) network fabric using libfabric

## Build:
Clone the repository, and make sure all the prerequisites are installed.

**Prerequisites**
- g++ >= 7.0
- cmake >= 3.10
- [gflags](https://github.com/gflags/gflags)
- [libfabric](https://ofiwg.github.io/libfabric/)

```shell
mkdir build && cd build
cmake ..
make
```

## Sample usage:

**Server**
```shell
./benchmark --flagfile=../server.conf
```

**Client**
```shell
./benchmark --flagfile=../client.conf
```