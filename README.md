# Covert Channel:  
---  
The basic idea is that the server will listen/close ports to transmit data to a client.   
  
The client will attempt to connect to the ports:  
  if the connection is successful then 1 has been transmitted,  
  else **ECONNREFUSED**, then 0 has been transmitted.  

The Server and the Client have been implemented in two modules each.  

## Server:  
First compile, `simple_keyconv.cpp` using: `g++ -std=c++11 simple_keyconv.cpp -o key`.  
This module will find `SSH` keys and print them to `stdout`.  

Then, compile `blink.cpp` using: `g++ -pthread -std=c++11 blink.cpp -o blink`.  
This module will actually transmit the `SSH` key.  

## Client:  
First, compile `bin2key.cpp` using: `g++ -std=c++11 bin2key.cpp -o bin2key`.  
This module will convert key from binary and print it out.  

The client, will also need `client.py`.  
This module is responsible for connecting to ports and interpreting the result.   
Please, change the `IP` global variable to match that of the server.  

## Run:  
After compiling, run the following commands:   

Server:  
`./key | ./blink`  

Client:   
`python client.py   && cat bin_file.txt | ./bin2key`  
