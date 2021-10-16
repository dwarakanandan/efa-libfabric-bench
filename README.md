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

Sample launch configuration can be found in `server.conf` and `client.conf`.

**Fabric Info**
```shell
./benchmark --fiinfo
```

**Server mode**
```shell
./benchmark --flagfile=../server.conf
```

**Client mode**
```shell
./benchmark --flagfile=../client.conf
```