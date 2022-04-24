#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "./include/http.h"

#define PORT 1234
#define MAX_RECV_SIZE 8192
#define MAX_SEND_SIZE 8192

int getSocket(char *address, int port)
{
  int sock = 0;
  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0)
  {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection Failed \n");
    return -1;
  }

  return sock;
}

HTTPRequest requestGET(std::string url)
{
  HTTPRequest request;
  request.method = "GET";
  request.URL = url;
  request.version = "HTTP/1.0";

  request.headers["Connection"] = "keep-alive";
  // request.headers["Content-Length"] = std::to_string(request.body.size());

  return request;
}

int main()
{
  int sock = -1, valread;
  char buffer[MAX_RECV_SIZE] = {0};

  while (true)
  {
    if (sock == -1)
      sock = getSocket("192.168.0.10", PORT);
    if (sock == -1)
      return -1;

    std::cout << "";
    int t;
    std::cin >> t;

    HTTPRequest request = requestGET("/");

    std::string request_message = request.toMessage();

    send(sock, request_message.c_str(), request_message.size(), 0);

    memset(buffer, 0, sizeof(buffer));
    valread = read(sock, buffer, MAX_RECV_SIZE);
    printf("%s\n", buffer);

    HTTPResponse response = HTTPResponse(buffer);
    if (response.headers["Connection"] == "close")
    {
      close(sock);
      sock = -1;
    }
  }

  close(sock);

  return 0;
}