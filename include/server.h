#ifndef SERVER_H
#define SERVER_H

#include "queryprocessor.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class Server {
public:
    Server(int port, QueryProcessor& queryProcessor) : port(port), queryProcessor(queryProcessor) {}

    void start() {
        createSocket();
        bindSocket();
        listenForConnections();
    }

    void addHandler(const std::string& route, std::function<std::string(const std::string&)> handler) {
        routeHandlers[route] = handler;
    }

private:
    int port;
    int serverSocket;
    QueryProcessor& queryProcessor;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> routeHandlers;

    std::string serveStaticFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string mime_type = "image/png"; // Set the appropriate MIME type for the file

        // Construct the HTTP response with the file content and MIME type
        return "HTTP/1.1 200 OK\r\nContent-Type: " + mime_type + "\r\nContent-Length: " + std::to_string(content.length()) + "\r\n\r\n" + content;
    }

    void createSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Error creating socket." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void bindSocket() {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error binding socket." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void listenForConnections() {
        if (listen(serverSocket, 10) == 0)
            std::cout << "Listening on port " << port << "...\n";
        else {
            std::cerr << "Error listening on port " << port << "...\n";
            exit(EXIT_FAILURE);
        }

        sockaddr_storage serverStorage;
        socklen_t addr_size = sizeof serverStorage;

        while (true) {
            int newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);

            if (newSocket >= 0) {
                handleRequest(newSocket);
                close(newSocket);
            }
        }
    }

    std::string handleRequest(int clientSocket) {
        char buffer[1024];
        recv(clientSocket, buffer, sizeof(buffer), 0);
        std::string clientRequest = buffer;

        // Parse the request to extract the HTTP method and path
        std::istringstream requestStream(clientRequest);
        std::string requestLine;
        std::getline(requestStream, requestLine);
        std::istringstream requestLineStream(requestLine);
        std::string httpMethod, path;
        requestLineStream >> httpMethod >> path;

        if (path.substr(0, 7) == "/images") {
            // Serve the image from your images directory
            std::string imagePath = "images" + path.substr(7); // Remove "/images" prefix from the path
            std::string response = serveStaticFile(imagePath);
            send(clientSocket, response.c_str(), response.length(), 0);
            return response;
        }

        // Process the client request based on the requested path
        if (routeHandlers.find(path) != routeHandlers.end()) {
            // Use the registered handler for the requested path
            std::string response = routeHandlers[path](clientRequest);

            // Send the response back to the client
            send(clientSocket, response.c_str(), response.length(), 0);

            // Return the response
            return response;
        } else {
            // Handle unknown routes with a 404 response
            std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(clientSocket, notFoundResponse.c_str(), notFoundResponse.length(), 0);

            // Return the 404 response
            return notFoundResponse;
        }
    }

};

#endif // SERVER_H