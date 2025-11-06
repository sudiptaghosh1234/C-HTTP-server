A simple multithreaded HTTP server written in C
# Simple HTTP Server in C

A lightweight, multithreaded HTTP server written in C using sockets and pthreads.

## Features
- Handles multiple clients simultaneously
- Supports basic GET routes (`/hello`, `/echo/...`)
- Demonstrates socket programming and concurrency

## How to run
```bash
gcc server.c -o server -lpthread
./server
