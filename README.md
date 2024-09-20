# Process Scheduler

## Introduction

A CPU scheduler is responsible for determining the execution order of processes within a system. It tracks the status of processes, ensuring optimal use of CPU resources and memory. When multiple processes are scheduled for execution, the scheduler decides which process to run next based on the scheduling algorithm in use. The goal is to maintain efficiency and minimize system overhead by managing processes in various states: **Running**, **Ready**, or **Blocked** (waiting on I/O or other resources).

This project implements a CPU scheduling system on a single-core CPU with infinite memory. The scheduler works with various scheduling algorithms, emulating how an operating system would handle process scheduling, timing, and memory management. The implemented scheduling algorithms are:

- **Highest Priority First (HPF)** (non-preemptive).
- **Shortest Remaining Time Next (SRTN)** (preemptive).
- **Round Robin (RR)** (with time quantum).

Additionally, a **Buddy Memory Allocation System** is integrated, managing memory allocation and deallocation for processes dynamically as they enter and exit the system.

---

## Project Components

### 1. **Process Generator**

The **process generator** is responsible for simulating the creation of processes and initiating the scheduler. It reads input files, sets up the system’s processes, and communicates with the scheduler to manage process flow. Key functionalities include:
- Reading input files that define process attributes such as **arrival time**, **runtime**, **priority**, and **memory size**.
- Asking the user for the chosen scheduling algorithm and its parameters.
- Creating a process data structure and handling inter-process communication (IPC).
- Sending process information to the scheduler when processes are ready.
- Clearing IPC resources at the end of the simulation.

### 2. **Clock Module**

The **clock module** acts as an integer time clock to simulate the system’s time. It ticks in discrete units, which the scheduler uses to measure and manage process execution time. This module is already provided.

### 3. **Scheduler**

The **scheduler** is the core component that implements the three scheduling algorithms: HPF, SRTN, and RR. The scheduler manages process states, allocates CPU time, and logs execution details. It uses **Process Control Blocks (PCB)** to store information such as **process state**, **execution time**, and **waiting time**. The scheduler performs the following tasks:
- Starts, pauses, and resumes processes based on the scheduling algorithm.
- Logs key metrics, such as **CPU utilization**, **turnaround time**, and **waiting time**.
- Produces output logs: `scheduler.log` and `scheduler.perf`, detailing process execution and performance metrics.

### 4. **Buddy Memory Allocation System**

The memory management system allocates memory to processes using the **Buddy Allocation** algorithm. It divides memory into blocks and allocates the smallest suitable block for each process. Memory is freed when the process completes. The memory size of each process is defined in the input, and the memory allocation is tracked in the `memory.log` file.

### 5. **Process Module**

The **process module** simulates the behavior of a process during execution. It interacts with the scheduler, notifying it upon termination. Each process behaves as if it is CPU-bound, continuously consuming CPU resources until its task is complete.

---

## Output Files

- **scheduler.log**: Logs each process's state transitions, such as start, stop, resume, and finish, along with relevant metrics like total time, waiting time, and weighted turnaround time (WTA).
- **scheduler.perf**: Summarizes CPU utilization, average WTA, average waiting time, and standard deviation of WTA.
- **memory.log**: Tracks memory allocation and deallocation events, logging the exact memory regions allocated to or freed by each process.

---

## To run Project
1. Clone the repository to a directory of your choice.
  
2. Compile Process Schedule
     ```bash
       make build
      ```
3. Run Process Schedule
     ```bash
       make run
      ```
---

## Conclusion

This project demonstrates a simulation of CPU scheduling and memory management in an operating system. The implemented algorithms and memory allocation strategies help illustrate the challenges of process management and resource optimization in real-world systems. The goal is to achieve high CPU utilization and minimal process waiting time while effectively managing limited memory resources.

