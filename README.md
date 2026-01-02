# Kache

A Redis-inspired **in-memory key-value store** implemented in C++. Built to explore how high-performance cache servers work internally.

Kache is written completely from scratch without external databases or frameworks. The goal is to understand **systems-level design choices**, **performance trade-offs**, and **internal mechanics** behind Redis-like systems.

[Read my full blog post on Kache](https://sakkshm.me/blog/building-kache)


## Project Overview

- Implements a minimal Redis-like TCP server
- Supports hashmaps and sorted sets with TTL expiration
- Uses a non-blocking, event-driven architecture
- Stores all data in memory with predictable behavior
- Focuses on correctness, simplicity, and learning

This project is intentionally minimal and made as a learning exercise.

## Core Capabilities

- In-memory key-value storage
- Custom binary request-response protocol
- Non-blocking TCP server
- Hashmap operations (`set`, `get`, `del`)
- Sorted set operations with ordered queries
- Millisecond-precision key expiration (TTL)
- Timer-driven eviction using priority scheduling
- Background thread pool for safe asynchronous cleanup
- Zero third-party dependencies

## Benchmarks (Production)

| Metric          | Value         |
| --------------- | ------------- |
| Total requests  | 1,220,000     |
| Average latency | 0.0296 ms     |
| Min latency     | 0.0146 ms     |
| Max latency     | 0.5257 ms     |
| p95 latency     | 0.0496 ms     |
| p99 latency     | 0.0546 ms     |
| Throughput      | 116,541 req/s |

## Techniques Used

- Event-driven network programming with non-blocking sockets
- Custom binary protocol parsing and serialization
- In-memory data modeling
- Priority-based scheduling for TTL management
- Intrusive data structures to reduce allocations
- Thread pool for offloading non-critical work
- Cache-friendly memory layouts
- Deterministic resource cleanup
- Low-level performance analysis and debugging in C++

## Internals (High-Level)

### Server Architecture

- A single-threaded event loop handles:

  - Network I/O
  - Command decoding and dispatch
  - Timer processing

- A background thread pool handles:

  - Deferred object destruction
  - Cleanup outside latency-sensitive paths

This design mirrors real-world cache servers where the **hot path remains lock-free and predictable**.

### Key Expiration Strategy

- TTL metadata is stored separately from values
- Expiration timestamps are scheduled using a min-heap
- The event loop periodically evicts expired keys
- Memory cleanup is delegated to background workers

This avoids blocking the main loop while maintaining accurate expiration semantics.

### Sorted Set Design

- Maintains ordering by `(score, name)`
- Supports insertion, deletion, and range queries
- Designed to model Redis ZSet behavior conceptually

## Command Interface

| Command                                        | Description                                     |
| ---------------------------------------------- | ----------------------------------------------- |
| `set <key> <value>`                            | Set a value for a key                           |
| `get <key>`                                    | Retrieve the value of a key                     |
| `del <key>`                                    | Delete a key and its value                      |
| `expire <key> <time>`                          | Set a TTL for a key (time in milliseconds)      |
| `persist <key>`                                | Remove the TTL from a key                       |
| `zadd <key> <score> <name>`                    | Add a `(name, score)` pair to a sorted set      |
| `zrem <key> <name>`                            | Remove an entry from the sorted set             |
| `zscore <key> <name>`                          | Get the score associated with a name            |
| `zquery <key> <score> <name> <offset> <limit>` | Query a sorted set with ordering and pagination |

## Project Structure

```
kache
├── Makefile
├── README.md
└── src
    ├── avl.hpp
    ├── client.cpp
    ├── hashtable.hpp
    ├── heap.hpp
    ├── list.hpp
    ├── main.cpp
    ├── thread_pool.hpp
    ├── utils.hpp
    └── zset.hpp
```

## Build & Run

### Production Build

Compile and run the production server:

```bash
make prod
./build/prod/main
```

> Logging (debug messages) is disabled in production builds.

### Development Build (Dev)

Compile in development mode with debug symbols:

```bash
make dev
./build/dev/main
```

> Logging (debug messages) is enabled only in development builds.

### Test Client

Optionally, run the client to connect to the server:

```bash
./build/prod/client   # or ./build/dev/client
```

### Running the Benchmark (Production Only)

The benchmark will start the server in the background, run performance tests, and stop the server automatically:

```bash
make benchmark
```

Sample output:

```
Starting production server...
Main server PID: 12345
Benchmark Started!
Benchmark finished:
Total requests: 1220000
Average latency: 0.033 ms
p95 latency: 0.054 ms
Throughput: 103692 req/s
Benchmark complete, main server stopped.
```

> The benchmark only works with the production build. It automatically launches `main`, runs `benchmark`, and stops the server when finished.

## Future Work

- Persistence (AOF / snapshots)
- RESP protocol compatibility
- Pub/Sub
- Multi-threaded I/O
- Rigorous benchmarking and profiling
- E2E testing
