# Covert Channel:  
---  
The basic idea is that the server will listen/close ports to transmit data to a client.   
  
The client will attempt to connect to the ports:  
  - if the connection is successful then 1 has been transmitted,  
  - else **ECONNREFUSED**, then 0 has been transmitted.  

The Server and the Client have been implemented in two modules each. 

# Table of Contents:  
---   
1. [Requirements](#start)   
2. [Server](#server)  
3. [Client](#client)  
4. [Run](#run)  

## Requirements:  
<a id="start"></a>  
To begin we need to edit the `Makefile`. We need to modify the values of `SERV_IP` and `KEYPATH`.  
`SERV_IP` needs to be the IP of the server. `KEYPATH` the path to a private SSH key.  

First, please determine the IP of the machine that will act as the server.  
I suggest using `ifconfig` to determine to do this. Insert this IP into the `Makefile`:   

<p align = "center"> 
<img src="https://github.com/syuja/coveredcomm/blob/master/imgs/serv_ip.png" alt="Makefile">
</p>

Next, we need to create an SSH key pair and provide the location of the private SSH Key in  
the `Makefile`. To create an SSH key use the `sshkeygen -t rsa -b 1024`. Then, insert the path  
to the newly created private key in the `Makefile`:   

<p align = "center"> 
<img src="https://github.com/syuja/coveredcomm/blob/master/imgs/keypath.png" alt="keypath">
</p>
  
 Now, you are ready to begin running the program.  

## Server:  
<a id="server"></a>  
To run the server use `make runServer`.  
This will compile `keyconv.cpp` and `blink.cpp`, and execute them with proper arguments:   

    `./key $(NUM_PORTS) $(KEYPATH) | ./blink $(NUM_PORTS) $(WAIT_TIME)`  
 
The `keyconv.cpp` module finds the SSH key and prints them to `stdout`.  
It prints it out at the specified width.  

The `blink.cpp` module will wait for the client and transmit the SSH key.  

**Note:** The server needs to start before the client.  
## Client:  
<a id="client"></a>  
To run the client use `make runClient`.  
This will compile `client.cpp` and `bin2key.cpp`, and execute them with properp arguments:  

     `./client $(SERV_IP) $(NUM_PORTS) $(WAIT_TIME) $(SAVED) && ./bin2key $(SAVED)`  
  
The client also consists of two modules. `client.cpp` locates the ports being used by the server  
and receives the message sent by it.  The second module, `bin2key.cpp`, reads the received  
message and converts it from binary to ascii characters printing the received private SSH key.  


## Run:  
<a id="run"></a>  
To run, use the following commands:  

Server:  
    `make runServer`  

Client:   
    `make runClient`  

Input is a private SSH key on the server side, and the output is the same private SSH key on the client side.  

