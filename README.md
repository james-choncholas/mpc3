# mpc3

[![ci](https://github.com/james-choncholas/mpc3/actions/workflows/ci.yml/badge.svg)](https://github.com/james-choncholas/mpc3/actions/workflows/ci.yml)

## About
MPC Congestion Control (MPC^3) testing.

## Getting Started
Build the container yourself (optional)
```bash
./docker/build.sh
```

Run the sender
```bash
./docker/run.sh
./mpc3/build/src/net_bench_app/net_bench_app -s -a 127.0.0.1
```

Run the receiver
```bash
./docker/run.sh
./mpc3/build/src/net_bench_app/net_bench_app -r
```

## More Details

 * [Dependency Setup](README_dependencies.md)
 * [Building Details](README_building.md)
 * [Troubleshooting](README_troubleshooting.md)
 * [Docker](README_docker.md)
