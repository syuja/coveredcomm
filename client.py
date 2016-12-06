# Echo client program
import socket
import sys
import time

HOST = '192.168.0.103'    # The remote host
              # The same port as used by the server
s = None
listOfPorts = []
numberOfPorts = 8
blinkingPort = numberOfPorts - 1
blinkingPortNumber = 0
i = 0;
clockTick = 1
timeout = 5
isFinished = False
while not isFinished:
    print 'Loop starts'
    for port in range(1024, 2000): #65536
        if isFinished:
            break
        for res in socket.getaddrinfo(HOST, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res
            try:
                s = socket.socket(af, socktype, proto)
            except socket.error as msg:
                print 'Couldnt create the socket:' + msg
                s = None
                continue
            try:
                s.connect(sa)
                if i == blinkingPort:
                    s.close()
                    print 'Waiting ticks'
                    time.sleep(clockTick)
                    try:
                        s.connect(sa)
                        listOfPorts.pop(0)
                        listOfPorts.append(port)
                    except socket.error as msg:
                        blinkingPortNumber = port
                        s.close()
                        s = None
                        isFinished = True
                        break
                else:
                    i=i+1
                    listOfPorts.append(port)
                
            except socket.error as msg:
                if i == blinkingPort:
                    print 'Waiting ticks'
                    time.sleep(clockTick)
                    try:
                        s.connect(sa)
                        blinkingPortNumber = port
                        s.close()
                        s = None
                        isFinished = True
                        break
                    except socket.error as msg:
                        del listOfPorts[:]
                        i=0
                        s.close()
                        s = None
                        continue
                        continue    
                else:
                    del listOfPorts[:]
                    i=0
                    s.close()
                    s = None
                    continue
    if not isFinished:
        del listOfPorts[:]

print listOfPorts
print blinkingPortNumber


#we connect to all ports
sockets = []
for port in listOfPorts:
    for res in socket.getaddrinfo(HOST, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
        af, socktype, proto, canonname, sa = res
        try:
            s = None
            s = socket.socket(af, socktype, proto)
        except socket.error as msg:
            s = None
            continue
        try:
            s.connect(sa)
            sockets.append(s)
        except socket.error as msg:
            s.close()
            s = None
            continue
        break

for socketi in sockets:
    socketi.close()
#Server dies here
print 'Connection established, waiting...' 
time.sleep(timeout)



#We are receiving key!
print 'Setting up control socket...'
controlSocket = None
for res in socket.getaddrinfo(HOST, blinkingPortNumber, socket.AF_UNSPEC, socket.SOCK_STREAM):
        af_control, socktype_control, proto_control, canonname_control, sa_control = res
        try:
            controlSocket = socket.socket(af, socktype, proto)
        except socket.error as msg:
            print ' Fatal error'
        break
print 'Control socket set'


data = ""
while True:
    try:
        for res1 in socket.getaddrinfo(HOST, blinkingPortNumber, socket.AF_UNSPEC, socket.SOCK_STREAM):
            af_control, socktype_control, proto_control, canonname_control, sa_control = res1
            #print 'Connecting to control socket'
            controlSocket = socket.socket(af_control, socktype_control, proto_control)
            controlSocket.connect(sa_control)
            newline = ""
            for port in listOfPorts:
                #print 'port:' + str(port)
                for res in socket.getaddrinfo(HOST, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
                    af, socktype, proto, canonname, sa = res
                    try:
                        s = None
                        s = socket.socket(af, socktype, proto)
                    except socket.error as msg:
                        print 'Couldnt create sockets'
                        s = None
                        continue
                    try:
                        s.connect(sa)
                        newline = newline + "1"
                        #print 'Connected'
                    except socket.error as msg:
                        #print 'Didnt connect'
                        newline = newline + "0"
                        s.close()
                        s = None
                        continue
                    break
            print newline
            controlSocket.close()
            data = data + newline
    except socket.error as msg:
        print msg
        print 'Transmission completed'
        break
    time.sleep(clockTick)
print data
