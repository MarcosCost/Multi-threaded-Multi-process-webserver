# Multi-Threaded Web Server with IPC and Semaphores
**Sistemas Operativos - TP2**

A production-grade concurrent HTTP/1.1 web server implementing advanced process and thread synchronization using POSIX semaphores, shared memory, Unix Domain Sockets, and thread pools.

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [System Requirements](#system-requirements)
4. [Project Structure](#project-structure)
5. [Compilation](#compilation)
6. [Configuration](#configuration)
7. [Usage](#usage)
8. [Testing](#testing)
9. [Implementation Details](#implementation-details)
10. [Known Issues](#known-issues)
11. [Authors](#authors)
12. [Acknowledgments](#acknowledgments)

## Overview
This project implements a multi-process, multi-threaded HTTP/1.1 web server designed for high concurrency and robustness. It employs a hybrid architecture:
* **Process Management:** Master-worker architecture using `fork()` for fault isolation.
* **Inter-Process Communication (IPC):** * **Unix Domain Sockets (`socketpair`):** Used to pass file descriptors (connected sockets) from Master to Workers using `sendmsg`/`recvmsg`.
    * **Shared Memory:** Used for global server statistics and configuration.
    * **POSIX Named Semaphores:** Used for flow control (Producer-Consumer) and resource protection.
* **Thread Synchronization:** Pthread mutexes, condition variables, and reader-writer locks.
* **Resource Management:** Thread-safe LRU file cache.

### Architecture Flow
1. **Master Process:** Accepts TCP connections on port 8080. It acts as a *Producer*, placing work into the system.
2. **IPC Channel:** The connection File Descriptor is transferred to a Worker via a Unix Socket.
3. **Worker Processes:** 4 workers (default) act as *Consumers*. They receive the FD and push it to a local internal queue.
4. **Thread Pool:** 10 threads per worker (default) pick up requests from the local queue, check the LRU cache, and serve the content.

## Features

### Core Features
* **Multi-Process Architecture:** 1 Master + N Workers (Configurable).
* **Thread Pools:** Fixed-size thread pool per worker to handle concurrent requests efficiently.
* **HTTP/1.1 Support:** Implements `GET` and `HEAD` methods.
* **MIME Types:** Supports HTML, CSS, JS, Images (GIF, JPG, PNG), PDF, etc.
* **Custom Error Pages:** Custom 404 and 503 HTML error pages.

### Synchronization Features
* **POSIX Named Semaphores:** `/ws_empty`, `/ws_filled` for inter-process coordination.
* **Pthread Mutexes:** Protects the internal job queue of each worker.
* **Condition Variables:** Efficient waiting in the thread pool (avoids busy-waiting).
* **Reader-Writer Locks (`pthread_rwlock`):** Optimizes access to the LRU Cache (allows multiple simultaneous readers).

### Advanced Features
* **Thread-Safe LRU Cache:** In-memory caching using a **Hash Table** (for O(1) access) combined with a **Doubly Linked List** (for eviction policy).
* **Shared Statistics:** Real-time metrics (requests, bytes, active connections) stored in Shared Memory.
* **Graceful Shutdown:** Catches `SIGINT` to clean up resources, kill workers, and unlink semaphores.
* **Apache Combined Log Format:** Thread-safe logging to `access.log`.

## System Requirements
* **Operating System:** Linux (Ubuntu 20.04+ recommended).
* **Compiler:** GCC with C11 support.
* **Libraries:** `pthread`, `rt` (POSIX Real-time).
* **Tools:** GNU Make, Apache Bench (`ab`), Curl, Valgrind.

## Project Structure

```text
concurrent-http-server/
├── src/
│   ├── main.c           # Entry point & Signal handling
│   ├── master.c         # Master logic (Accept & sendmsg)
│   ├── worker.c         # Worker logic (recvmsg & local queue)
│   ├── thread_pool.c    # Thread pool implementation
│   ├── worker_queue.c   # Internal job queue implementation
│   ├── http.c           # HTTP Parsing & Response building
│   ├── cache.c          # LRU Cache (Hash + Linked List)
│   ├── logger.c         # Thread-safe logging
│   ├── stats.c          # Shared statistics management
│   ├── config.c         # Configuration loader
│   ├── shared_mem.c     # Shared memory setup
│   └── semaphores.c     # Semaphore initialization
├── tests/
│   ├── test_functional.sh # Functional tests script
│   ├── test_load.sh       # Load & Durability tests
│   └── test_concurrent.c  # Custom concurrent client
├── www/                 # Document Root
│   ├── index.html       # Bootstrap-based dashboard
│   ├── 404.html         # Custom Not Found page
│   ├── 503.html         # Custom Service Unavailable page
│   ├── css_ex.css       # Example Stylesheet
│   ├── son.gif          # Test Image
│   ├── kesha.jpg        # Test Image
│   ├── initiald.gif     # Test Image
│   └── KW.jpg           # Test Image
├── server.conf          # Configuration file
├── Makefile             # Build automation
└── README.md            # This file
````

## Compilation

### Quick Start

```bash
# Build the server
make

# Clean build artifacts
make clean

# Rebuild from scratch
make clean && make
```

### Build Targets

* `make all`: Builds the server and test client (default).
* `make debug`: Builds with debug symbols (`-g`) and defines `DEBUG`.
* `make release`: Builds with optimization (`-O3`).
* `make run`: Builds and starts the server immediately.
* `make test`: Runs the full test suite.
* `make valgrind`: Runs the server under Valgrind (memcheck) for leak detection.
* `make helgrind`: Runs the server under Helgrind for race condition detection.

## Configuration

The server reads `server.conf` from the root directory.

```ini
PORT=8080
DOCUMENT_ROOT=www/
NUM_WORKERS=4
THREADS_PER_WORKER=10
MAX_QUEUE_SIZE=100
LOG_FILE=access.log
CACHE_SIZE_MB=10
TIMEOUT_SECONDS=30
```

## Usage

### Starting the Server

```bash
./server
```

*To stop the server safely, press `Ctrl+C`. This triggers the cleanup routine.*

### Accessing Content

Open your browser or use `curl`:

* **Dashboard:** `http://localhost:8080/index.html`
* **Images:** \* `http://localhost:8080/son.gif`
    * `http://localhost:8080/initiald.gif`
* **Styles:** `http://localhost:8080/css_ex.css`
* **Test 404:** `http://localhost:8080/missing_file.html`

## Testing

### Automated Tests

Run the full suite (Functional + Load + Concurrency):

```bash
make test
```

### Manual Load Testing

Using Apache Bench (`ab`):

```bash
# 10,000 requests with 100 concurrent connections
ab -n 10000 -c 100 http://localhost:8080/index.html
```

## Implementation Details

### Master Process (Producer)

1.  Initializes Shared Memory and Semaphores.
2.  Enters an infinite loop calling `accept()`.
3.  Checks the `empty_slots` semaphore to ensure the system isn't overloaded.
4.  Uses **Unix Domain Sockets** (`socketpair`) and `sendmsg` with `SCM_RIGHTS` to pass the client File Descriptor to a worker.
5.  Signals `filled_slots`.

### Worker & Thread Pool (Consumer)

1.  Waits on `filled_slots`.
2.  Receives the FD via `recvmsg` from the IPC socket.
3.  Pushes the job to a local `worker_queue` protected by a Mutex.
4.  Threads in the pool wake up (Condition Variable), dequeue the FD, and process the request.

### LRU Cache

Implemented using a **Hash Table** (djb2 algorithm) for fast lookups and a **Doubly Linked List** to track usage order.

* **Synchronization:** Uses `pthread_rwlock_t`. Multiple threads can read from the cache simultaneously (`rdlock`), but eviction or insertion requires exclusive access (`wrlock`).

## Known Issues

* **No Keep-Alive:** Connections are closed after every request.
* **No HTTPS:** Traffic is unencrypted.
* **Queue Saturation:** Extreme loads (\> system limits) might result in dropped connections if the OS backlog fills up.

## Authors

* **Marcos Costa** (NMec: 125882)
* **José Mendes** (NMec: 114429)

## Acknowledgments

### Bibliographic References

* **[The Linux Programming Interface](https://man7.org/tlpi/)** (Michael Kerrisk) - *Referência fundamental para a implementação da passagem de descritores de ficheiro (SCM\_RIGHTS) e Sockets Unix.*
* **[Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)** (Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau) - *Conceitos de concorrência e virtualização.*
* **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)** - *Guia essencial para a utilização de sockets e estruturas de rede.*

### Technical Standards & Documentation

* **[HTTP/1.1 RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616)** - *Especificação oficial do protocolo para implementação dos métodos GET/HEAD e códigos de estado.*
* **[POSIX.1-2017 Standard](https://pubs.opengroup.org/onlinepubs/9699919799/)** - *Documentação oficial para `semaphore.h` (sem\_open) e `pthread.h`.*
* **Linux Man Pages** - *Documentação específica do sistema:*
    * [`unix(7)`](https://man7.org/linux/man-pages/man7/unix.7.html): *Sockets para IPC local.*
    * [`cmsg(3)`](https://man7.org/linux/man-pages/man3/cmsg.3.html): *Dados auxiliares para envio de FDs.*
    * [`shm_overview(7)`](https://man7.org/linux/man-pages/man7/shm_overview.7.html): *Memória partilhada POSIX.*
* **[POSIX Threads Programming (LLNL)](https://hpc-tutorials.llnl.gov/posix/)** - *Tutorial para gestão de pthreads e mutexes.*

### Algorithms

* **[djb2 Hash Function](http://www.cse.yorku.ca/~oz/hash.html)** (Daniel J. Bernstein, 1991) - *Algoritmo de hash eficiente utilizado na implementação da Hash Table da Cache LRU.*

### Online Resources & Tutorials
* **[StackOverflow: How to use sendmsg to send a file descriptor](https://stackoverflow.com/questions/8481138/how-to-use-sendmsg-to-send-a-file-descriptor-via-sockets-between-2-processes)** - *Principal fonte utilizada para implementar a função `send_fd` e entender a estrutura `msghdr`.*
* **[IBM Documentation: socketpair()](https://www.ibm.com/docs/en/ztpf/1.1.2025?topic=apis-socketpair-create-pair-connected-sockets)** - *Guia utilizado para configurar a comunicação bidirecional entre Master e Workers.*
* **[IBM Documentation: Worker program example](https://www.ibm.com/docs/en/i/7.6.0?topic=processes-example-worker-program-used-sendmsg-recvmsg)** - *Referência prática para a lógica de receção de descritores (`recvmsg`) no lado do processo Worker.*
* **[Quora: Passing file descriptors between processes](https://www.quora.com/How-can-you-pass-file-descriptors-between-processes-in-Unix)** - *Utilizado para compreender o conceito teórico de transferência de sockets em sistemas UNIX.*
* **[djb2 Hash Function](http://www.cse.yorku.ca/~oz/hash.html)** - *Algoritmo utilizado na implementação da Hash Table da Cache LRU.*

### AI Assistance
* **Generative AI Tools** - *Utilizadas como ferramenta de apoio para debugging, correção de erros de sintaxe e revisão de documentação.*

### Course Information

* **Course:** Sistemas Operativos (40381-SO)
* **Semester:** 1º Semester 2025/2026
* **Instructors:** Prof. Pedro Azevedo Fernandes / Prof. Nuno Lau
* **Institution:** Universidade de Aveiro
