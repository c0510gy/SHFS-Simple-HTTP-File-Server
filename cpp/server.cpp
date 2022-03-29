#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 1234

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
    std::cerr << "socket failed";
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    std::cerr << "bind failed";
    return -1;
  }

  if (listen(server_fd, 3) < 0)
  {
    std::cerr << "listen";
    return -1;
  }

  if ((listen_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
  {
    std::cerr << "accept";
    return -1;
  }

  valread = read(listen_fd, buffer, 1024);
  printf("%s\n", buffer);
  send(listen_fd, hello, strlen(hello), 0);
  printf("Hello message sent\n");

  return 0;
}