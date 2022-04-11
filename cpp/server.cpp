#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
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

HTTPResponse errorResponse(std::string error_code, std::string error_phrase, std::string error_message)
{
  HTTPResponse response;
  response.protocol = "HTTP/1.0";
  response.status_code = error_code;
  response.status_phrase = error_phrase;

  response.headers["Connection"] = "close";
  response.body = "<html><title>Tiny Error</title><body style=\"background-color:"
                  "ffffff\""
                  ">" +
                  error_message + "</body></html>";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
}

std::string getType(std::string filepath)
{
  if (filepath.find(".html") != std::string::npos)
    return "text/html";
  else if (filepath.find(".gif") != std::string::npos)
    return "image/gif";
  else if (filepath.find(".png") != std::string::npos)
    return "image/png";
  else if (filepath.find(".jpg") != std::string::npos)
    return "image/jpeg";
  else if (filepath.find(".mp4") != std::string::npos)
    return "image/mp4";

  return "text/plain";
}

std::string parsePath(std::string url)
{
  std::string path = "";
  for (int j = 0; j < url.size() && url[j] != '?'; ++j)
  {
    path += url[j];
  }
  if (path == "/")
  {
    path += "index.html";
  }
  return path;
}

HTTPResponse requestHandler(char *buffer)
{
  string req_message = string(buffer);

  HTTPRequest request = HTTPRequest(req_message);

  string filepath = "./root" + parsePath(request.URL);
  std::cout << filepath << std::endl;

  if (!std::filesystem::exists(filepath))
  {
    return errorResponse("404", "Not found", "Couldn't find this file");
  }

  HTTPResponse response = HTTPResponse();
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";

  response.headers["Connection"] = "close";
  response.headers["Content-type"] = getType(filepath);
  response.body = readFile(filepath);
  // response.body = "<html><head><title>example</title></head><body>Hello, world!</body></html>";
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

  while (true)
  {
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

    valread = read(listen_fd, buffer, 1024);
    printf("%s\n", buffer);
    HTTPResponse response = requestHandler(buffer);
    string response_message = response.toMessage();
    // std::cout << response_message << std::endl;
    send(listen_fd, response_message.c_str(), response_message.size() + 1, 0);
  }

  return 0;
}