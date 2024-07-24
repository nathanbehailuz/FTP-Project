# File Transport Protocol Project

## Overview

This project demonstrates a simple client-server network communication using C. The client connects to the server and exchanges messages. The server listens for incoming connections and responds to client requests.

## Files

- `client.c`: Contains the implementation of the client-side application.
- `server.c`: Contains the implementation of the server-side application.
- `server.h`: Contains the header file for the server-side application.

## Getting Started

### Prerequisites

- GCC compiler
- Make utility (optional, for ease of compilation)

### Compilation

To compile the client and server programs, you can simply run:

```sh
make
```

### Running the Programs

1. **Start the Server:**

   ```sh
   ./server
   ```

2. **Run the Client:**

   ```sh
   ./client
   ```

## Usage

The server will listen on port 8080 for incoming connections. The client will connect to the server on the same port and send a "Hello from client" message. The server will respond with a message and print it on the console.

## Code Structure

- `client.c`:
  - Establishes a connection to the server.
  - Sends a message to the server.
  - Receives and prints the response from the server.

- `server.c`:
  - Listens for incoming client connections.
  - Accepts a connection and reads the client's message.
  - Sends a response back to the client.
