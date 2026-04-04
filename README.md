# Multi-Process-Parallel-File-Keyword-Searcher

## Overview

This project is a **multi-process parallel file keyword searcher** written in C. It searches for a user-specified keyword across multiple text files and uses **process-based parallelism** to improve efficiency. The parent process creates worker processes, distributes files among them, and collects the search results through inter-process communication.

The program demonstrates key systems programming concepts including:

* process creation with `fork()`
* inter-process communication using pipes
* synchronization between parent and child processes
* file I/O
* process cleanup with `wait()` / `waitpid()`

---

## Features

* Search for a keyword in multiple files
* Use multiple worker processes in parallel
* Parent process coordinates work distribution
* Workers process files independently
* Results are sent back to the parent through pipes
* Final matches are displayed clearly to the user

---

## How It Works

The program follows a **parent-worker model**:

1. The **parent process** starts and creates worker processes using `fork()`.
2. Each worker is assigned one or more files to search.
3. Workers open their assigned files and search for the target keyword.
4. Each worker sends its results back to the parent using a pipe.
5. The parent collects the outputs from all workers.
6. After all results are gathered, the parent waits for all child processes to terminate.

This allows multiple files to be searched at the same time instead of one after another.

---

## File Structure

Example project structure:

```text
.
├── Makefile
├── README.md
├── project.pdf
├── video.txt
├── util.c
├── util.h
├── worker.c
├── worker.h
├── pfind.c
├── test_worker.c
├── test_empty.txt
├── test1.txt
├── test2.txt
├── test3.txt
├── test4.txt
└── test_worker
```

### Main files

* `pfind.c` — main program / parent process logic
* `worker.c` — worker process logic
* `util.c` — helper functions
* `*.h` — header files
* `Makefile` — build instructions
* `test*.txt` — sample test input files

---

## Compilation

To compile the project, run:

```bash
make
```

This should generate the executable.

If needed, you can also clean build files with:

```bash
make clean
```

---

## Usage

Run the program with:

```bash
./pfind <keyword> <file1> <file2> ...
```

### Example

```bash
./pfind hello test1.txt test2.txt test3.txt
```

This searches for the word `hello` in the listed files.

---

## Output

The program outputs the lines or files where the keyword is found, depending on your implementation.

Example:

```text
test1.txt: hello world
test3.txt: say hello again
```

If no match is found, the program may print a message indicating that no results were found.

---

## Concurrency Model

This project uses **multi-process concurrency** rather than threads.

* The parent process forks child worker processes.
* Each worker searches independently in its assigned file(s).
* Pipes are used for communication between workers and the parent.
* The parent reads results from the pipes and combines them.
* The parent uses `wait()` or `waitpid()` to collect all child processes and avoid zombies.

This design keeps the concurrency model simple and relies on operating-system-level process isolation.

---

## Inter-Process Communication

Communication between processes is handled using **pipes**.

* The parent creates a pipe before forking.
* Each child writes its search results into the pipe.
* The parent reads from the pipe after workers finish or as results become available.

This makes it possible to pass information safely from child processes back to the parent.

---

## Limitations

* Designed primarily for text files
* Exact behavior depends on keyword-matching logic
* Performance depends on number of processes and file sizes
* Output order may vary depending on process scheduling

---

## Author
Ziheng Wang

Yifei Yang

Kaiwen Yang

