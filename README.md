# Introduction

Proof of concept implementation of Peer-to-peer file transfer in C++. Server application has files that clients can download. Upon file download request server loads file into RAM memory and returns response to client indicating how many parts file has been divided into and if some parts can be downloaded from other clients. Server then starts sending file parts to client, while client attempts to contact other clients that were specified by server as file owners in attempt to download file parts from them concurrently. 

File transfer communication protocol is implemented above TCP/IP with specific message formats that must be sent in order for communication to work. All TCP sockets are in non blocking mode and are multiplexed by events. Every new connected client and his request are concurrently handled by separate server threads, and every new peer to peer connection is also handled in the same way by clients. Common protocol functions are separated in different static CPP libraries.

Several stress tests were run in order to document and analyse system performance. Stress test results as well as documentation in Serbian are available in IKP-Dokumentacija.pdf.

## Technologies

- C/C++
- Winsock library

## How to run this?

- Run Server/Server.cpp
- Run multiple clients on localhost Client/Client.cpp
- Input file names to be downloaded in client application
- Let server do it's thing .. :)

## Authors
Predrag Glavas and Stefan Besovic

## License
[MIT](https://choosealicense.com/licenses/mit/)
