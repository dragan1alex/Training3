# Training3
Sockets playground

Here you'll find a socket that serves local files via a given port and a client that requests files from the server.

(server)
Run as root with the port given as an argument
 EX: sudo ./server 123

(client)
You can run as a normal user with the following arguments: IP, PORT, FileName
 EX: ./client localhost 123 test.png


The server will serve the file in several chunks (1024 bytes each) to minimize the memory used by the apps. 

After each chunk the server checks with the client if it has received the expected ammount of data and retries if the client received garbage.

ToDo: some kind of checksum must be implemented if data integrity is a must. The server only checks if the client received "X" amount of data, not if the data received was altered along the way.
