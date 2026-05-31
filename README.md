# POSIX Multiprocess Synchronization & Communication System

A low-level concurrency project implemented in **C (POSIX)** demonstrating advanced process management, inter-process communication (IPC) via UNIX pipes, and robust synchronous runtime handling.

## 🚀 Overview

This repository contains an academic proof-of-concept focused on operating system architecture, deterministic process lifecycles, and secure data pipeline routing. The project is divided into two distinct core activities:

1. **Activity 1 (Basic Message Transformation):** Establishes a bidirectional parent-child topology where parallel worker processes apply basic cryptographic transformations to isolated data streams via dual-pipe routing.
2. **Activity 2 (Timed Window Stream Processor):** Features a complex three-stage multiprocess pipeline (Capturer, Detector, and Final Processor) that dynamically inspects data streams in real-time, shifting storage destinations based on a 10-second reactive timed window triggered by stream tokens (`@`).

---

## 🛠️ System Architecture & Workflow

The pipeline orchestrates 4 concurrent processes utilizing unidirectional UNIX pipes and strict resource cleanup to eliminate side effects:

* **Process Parent (Orchestrator):** Manages global system state, intercepts signals, and monitors children asynchronously to prevent zombie or orphan states.
* **Process A (Capturer):** Non-blocking user interaction stage. Streams lines from standard input into the main IPC pipeline until a termination token (`SORTIR`) is issued.
* **Process B (Detector):** The core routing engine. It scans the stream using `strchr()`. When an `@` token is intercepted, it calculates an active window (`time(NULL) + 10s`). It applies a Caesar cipher shift and conditionally forks data downstream to Process C (via `pipe_BC`) or writes to a local standalone log file.
* **Process C (Final Processor):** An isolated, data-sink stage that continuously flushes processed information received from Process B into a persistent storage file (`arxiu_procesC.txt`).

---

## 🔒 Concurrency & Signal Handling

To ensure deterministic execution and prevent OS resource leaks, specific low-level strategies were implemented:
* **Asynchronous Monitoring:** The parent process leverages `waitpid()` coupled with the `WNOHANG` flag to regularly poll the status of Process A without blocking CPU execution.
* **Graceful Cascaded Shutdown:** Built-in signal handling via `sigaction` intercepts `SIGINT` (Ctrl+C), prompting the parent to actively broadcast `SIGTERM` across the process tree before reaping dead descriptors.
* **Deadlock Avoidance:** Unused reading and writing file descriptors (`fd`) from the pipe arrays are explicitly closed early inside each child scope to guarantee proper End-Of-File (EOF) signals.

---

## 💻 Compilation & Execution

This project is strictly compliant with the **C11 standard** and requires POSIX macros (`_XOPEN_SOURCE 700`) for system call feature test definitions.

### Compilation
Compile the source files using `gcc` with strict warning flags:
```bash
# Activity 1
gcc -Wall -Wextra -std=c11 activitat1.c -o activitat1

# Activity 2
gcc -Wall -Wextra -std=c11 activitat2.c -o activitat2
