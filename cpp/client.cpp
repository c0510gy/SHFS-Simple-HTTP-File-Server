#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "./include/http.h"
#include "./include/base64.h"

#define PORT 1234
#define MAX_RECV_SIZE 8192
#define MAX_SEND_SIZE 8192

int getSocket(char *address, int port);

HTTPRequest requestGET(std::string url);
HTTPRequest requestHEAD(std::string url);
HTTPRequest requestPOST(std::string url, std::string directory_name);
HTTPRequest requestDELETE(std::string url, std::string file_name);
HTTPRequest requestPUT(std::string url, std::string file_name, std::string file_content);

std::string readFile(std::string filepath);

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

    std::cout << "Commands: ";
    std::cout << "\n\t- Get file or list directory\n\t\tget <path>";
    std::cout << "\n\t- Head request (GET method response without body)\n\t\thead <path>";
    std::cout << "\n\t- Create new directory\n\t\tcreate_dir <path> <new directory name>";
    std::cout << "\n\t- Create new file\n\t\tcreate_file <path> <file name> <content>";
    std::cout << "\n\t- Delete file or directory\n\t\tdel <path> <file name>";
    std::cout << "\n\n";

    HTTPRequest request;

    std::string comd, path;
    std::cout << "Command> ";
    std::cin >> comd >> path;
    if (comd == "get")
    {
      request = requestGET(path);
    }
    else if (comd == "head")
    {
      request = requestHEAD(path);
    }
    else if (comd == "create_dir")
    {
      std::string directory_name;
      std::cin >> directory_name;
      request = requestPOST(path, directory_name);
    }
    else if (comd == "create_file")
    {
      std::string file_name, file_content;
      std::cin >> file_name >> file_content;
      request = requestPUT(path, file_name, file_content);
    }
    else if (comd == "del")
    {
      std::string file_name;
      std::cin >> file_name;
      request = requestDELETE(path, file_name);
    }
    else
    {
      std::cout << "Wrong command\n";
      continue;
    }

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

  return request;
}

HTTPRequest requestHEAD(std::string url)
{
  HTTPRequest request = requestGET(url);
  request.method = "HEAD";

  return request;
}

HTTPRequest requestPOST(std::string url, std::string directory_name)
{
  HTTPRequest request;
  request.method = "POST";
  request.URL = url;
  request.version = "HTTP/1.0";

  request.headers["Connection"] = "keep-alive";
  request.body = "directory_name=" + urlEncode(directory_name);
  request.headers["Content-Length"] = std::to_string(request.body.size());

  return request;
}

HTTPRequest requestDELETE(std::string url, std::string file_name)
{
  HTTPRequest request;
  request.method = "DELETE";
  request.URL = url;
  request.version = "HTTP/1.0";

  request.headers["Connection"] = "keep-alive";
  request.body = "file_name=" + urlEncode(file_name);
  request.headers["Content-Length"] = std::to_string(request.body.size());

  return request;
}

HTTPRequest requestPUT(std::string url, std::string file_name, std::string file_content)
{
  HTTPRequest request;
  request.method = "PUT";
  request.URL = url;
  request.version = "HTTP/1.0";

  request.headers["Connection"] = "keep-alive";
  request.body = "file=" + urlEncode(base64Encode(file_content));
  request.body += "&name=" + urlEncode(file_name);
  request.headers["Content-Length"] = std::to_string(request.body.size());

  return request;
}

std::string readFile(std::string filepath)
{
  std::ifstream file;
  file.open(filepath.c_str());
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::stringstream strStream;
  strStream << file.rdbuf();

  return strStream.str();
}
