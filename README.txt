# Authors
- The NGUYEN
- Minh PHAM

# Compilation:
- To compile both client and server: $ make all
- To compile only the client: $ make client
- To compile only the server: $ make server
- To remove all the object files and executable files: $ make clean

# Run the program:
- First run the server: $ ./server <port_number>
    - <port_number> should be >= 1024 and <= 65535.
    - E.g: $ ./server 8080
- Then run the client: $ ./client <IP_address> <port_number>
    - <IP_address> should be the IPv4 address of the host of the server, and <port_number> should be the same as the one used to run the server.
    - E.g: $ ./client 127.0.0.1 8080
- Clients will automatically connect to the server, and if there are enough 2 players, the server will start the game.
- Have fun!