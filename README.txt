# Compilation:

- To compile both client and server: $ make all
- To compile only the client: $ make client
- To compile only the server: $ make server
- To remove all the object files and executable files: $ make clean

# Run the program:
- First run the server: $ ./server <port_number> 
    - E.g: port_number should be >= 1024 and <= 65535: $ ./server 8080
- Then run the client: $ ./client <IP address> <port_number>
    - IP address should be the IP address of the host of the server, and pirt number should be the same as the one used to run the server.
    - E.g: $ ./client 127.0.0.1 8080
- Client will be automatically connected to client and if there are enough 2 players, server will start the game. The player 1 starts. 
- Have fun!