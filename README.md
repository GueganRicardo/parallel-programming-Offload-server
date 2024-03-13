# Operating Systems Final Project: Task Offloading Simulator

## Authors
- Ricardo Guegan
- Samuel Machado

## Overview
This project presents a simulator for task offloading, addressing concurrency issues, resource sharing, and synchronization problems in a system with multiple edge servers.

## Approach
- **Concurrency**: Implemented individual data structures for each edge server, regulated by mutexes to minimize competition for write access.
- **Resource Sharing**: Utilized mutexes to manage access to the common log file and stats struct, which are critical points of concurrency.
- **Synchronization**: Employed condition variables and semaphores to synchronize the Task Manager, Scheduler, and Dispatcher, ensuring tasks are efficiently received, placed, and delivered.

## Task Management
- **SPN Algorithm**: The scheduler uses the Shortest Process Next (SPN) algorithm to maximize the number of tasks completed in a timely manner.
- **Task Validity**: Regularly re-evaluates the validity of tasks in the queue and their feasibility based on the availability and power of the virtual CPUs (vCPUs).
