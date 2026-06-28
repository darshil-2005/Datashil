# Datashil

Datashil is a lightweight, disk-based database engine written in modern C++. It implements a persistent B+ tree storage engine, a custom binary client/server protocol over TCP, and an interactive CLI for executing basic database operations. The project is designed to explore the core components of a database system, including storage management, indexing, buffering, and networking.

## Features

- Persistent disk-backed B+ tree index
- Slotted-page storage format with page defragmentation
- Buffer pool for cached page management
- TCP client/server architecture with a custom binary protocol
- Interactive CLI supporting insert, search, and delete operations
- Comprehensive Catch2 test suite covering persistence, insertion, search, deletion, and stress testing

## Repository Layout

- `commons/` shared types, constants, and utilities
- `database/` storage engine, server, and tests
- `client/` tcp client for server interaction with a simple cli for demonstration

## Prerequisites

- `g++` with C++17 and above supported
- `make`
- Python 3 for generating test data

## Build

Build both the server and client binaries:

```bash
make
```

This produces:

- `server_app`
- `client_app`

Clean generated binaries and object files:

```bash
make clean
```

## Run the Server

Start the database server with a path to the database file or directory:

```bash
./server_app <database_path> [port]
```

The database file is formed along with all its parent folders if needed.

## Run the Client

The client includes a simple interactive CLI for demonstration purposes.

Connect the client-CLI to the server with a host and port:

```bash
./client_app <host> <port>
```

Once connected, enter commands at the `datashil>` prompt.

### Supported Commands

- A simple parser is implemented to parse the queries.
- Commands are terminated with a semicolon and use a simple grammar.

```text
search <key>;
insert <key> "<payload>";
delete <key>;
```

**Example session:**
<div>
<img src="static/demonstration.png" width="500">
</div>

## Tests

Generate the test datasets and run the test binary with:

```bash
make generate_data
make test
```

The test suite covers:

- B+ tree insertion
- Search
- Deletion
- Persistence
- Page defragmentation

Stress tests include:

- Up to 60,000 inserted records
- Payloads up to 10,000 bytes
- 30,000 deletions using multiple deletion patterns

## Notes

- Test data is stored in `database/tests/data/`.
- Communication between the client and server uses a custom binary protocol with checksum validation.
