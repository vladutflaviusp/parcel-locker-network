# Parcel Locker Network

A C++ client-server project that simulates a parcel locker network, built from scratch to showcase low-level networking, local persistence, and clean architecture.

## What It Does

- **Concurrent TCP Server**: Uses native socket I/O multiplexing (`select()` via Winsock2) to handle multiple clients smoothly on a single thread.

- **Custom Binary Protocol**: Communicates using lightweight, direct binary structures for fast and efficient messaging between client and server.

- **SQLite3 Persistence**: Locker states, packages, and PIN codes are safely stored in a local database so data survives server restarts.

- **Clean Separation of Concerns**: The codebase is neatly split into distinct layers handling network events, business logic, and database operations.

## Tech Stack

- **Language**: C++
- **Networking**: Winsock2 (TCP Sockets)
- **Database**: SQLite3
- **Build System**: CMake
- **Version Control**: Git (managed with clean, atomic commits)

## How to Run It

### 1. Clone the repository

```bash
git clone https://github.com/vladutflaviusp/parcel-locker-network.git
cd parcel-locker-network
```

### 2. Compile the project using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 3. Start the server

```bash
./Server <port>
```

## Protocol Overview

Clients talk to the server by sending direct binary messages for three main actions:

- **Type 1 (Status Check)**: Checks if a specific locker is currently available or occupied.

- **Type 2 (Deposit)**: Attempts to reserve an empty locker and set a security PIN.

- **Type 3 (Pickup)**: Validates the PIN and frees up the locker upon successful collection.

The server processes each request and sends a binary response containing the operation status and, when applicable, the locker occupancy status.