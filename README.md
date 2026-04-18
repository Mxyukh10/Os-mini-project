# OS Mini Project: Supervisor CLI with FIFO IPC

## Overview

This project implements a simple container management system using a supervisor process and a command-line interface (CLI).

It simulates basic container operations such as starting, stopping, and listing containers. Communication between the CLI and the supervisor is implemented using Inter-Process Communication (IPC) through Named Pipes (FIFO).

---

## Features

* Create containers using `fork()`
* Stop containers using `kill()`
* List active containers with their name and PID
* Communication using FIFO (`/tmp/os_fifo`)
* Supervisor process runs continuously and manages all containers

---

## Project Structure

```
.
├── supervisor.c
├── cli.c
├── README.md
├── .gitignore
```

---

## Compilation

```bash
gcc -Wall -Wextra -std=c11 -o supervisor supervisor.c
gcc -Wall -Wextra -std=c11 -o cli cli.c
```

---

## How to Run

### 1. Start Supervisor (Terminal 1)

```bash
./supervisor
```

### 2. Run CLI Commands (Terminal 2)

Start a container:

```bash
./cli start alpha
```

Start another container:

```bash
./cli start beta
```

List running containers:

```bash
./cli list
```

Stop a container:

```bash
./cli stop alpha
```

---

## Sample Output

Supervisor terminal:

```
Supervisor started. Listening on /tmp/os_fifo
Supervisor: started container 'alpha' with PID 12345
Supervisor: started container 'beta' with PID 12346
Active containers:
Name: alpha, PID: 12345
Name: beta, PID: 12346
Supervisor: stop signal sent to 'alpha'
Supervisor: container 'alpha' exited
```

---

## Concepts Used

* Process creation using `fork()`
* Process termination using `kill()`
* Inter-Process Communication using FIFO
* Signal handling
* Basic process management

---

## Notes

* The supervisor must be running before using CLI commands
* Output is displayed in the supervisor terminal
* FIFO is created at `/tmp/os_fifo`

---


