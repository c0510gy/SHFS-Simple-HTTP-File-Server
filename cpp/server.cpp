#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "http.h"
#define PORT 1234

using std::cerr;
using std::map;
using std::string;

HTTPResponse requestHandler(char *buffer)
{
  string req_message = string(buffer);

  HTTPRequest request = HTTPRequest(req_message);
  HTTPResponse response = HTTPResponse();
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";

  response.headers["Connection"] = "close";
  response.body = "<html><head><title>example</title></head><body>Hello, world!</body></html>";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
}

int main()
{
  int server_fd, listen_fd, valread;
  int opt = 1;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};
  char *hello = "Hello from server";

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    cerr << "socket failed";
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    cerr << "bind failed";
    return -1;
  }

  if (listen(server_fd, 3) < 0)
  {
    cerr << "listen";
    return -1;
  }

  if ((listen_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
  {
    cerr << "accept";
    return -1;
  }

  while (true)
  {
    valread = read(listen_fd, buffer, 1024);
    printf("%s\n", buffer);
    HTTPResponse response = requestHandler(buffer);
    string response_message = response.toMessage();
    std::cout << response_message << std::endl;
    send(listen_fd, response_message.c_str(), response_message.size() + 1, 0);
    // send(listen_fd, hello, strlen(hello), 0);
    // printf("Hello message sent\n");
  }

  return 0;
}