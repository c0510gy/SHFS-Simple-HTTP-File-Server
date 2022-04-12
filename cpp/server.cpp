#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "./include/http.h"
#define FMT_HEADER_ONLY
#include "./lib/fmt/core.h"
#define PORT 1234

const std::string ROOT_DIRECTORY = "./root";

using std::cerr;
using std::map;
using std::string;

std::string current_directory(std::string path)
{
  std::string ret = "";

  int j = path.size();
  while (j && path[--j] != '/')
    ;
  for (int i = 0; i <= j; ++i)
    ret += path[i];
  return ret;
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

HTTPResponse errorResponse(std::string error_code, std::string error_phrase, std::string error_message)
{
  HTTPResponse response;
  response.protocol = "HTTP/1.0";
  response.status_code = error_code;
  response.status_phrase = error_phrase;

  std::string error_template = readFile("./templates/error.html");

  response.headers["Connection"] = "close";
  response.body = fmt::format(error_template, error_code, error_phrase, error_message);
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
    // path += "index.html";
  }
  return path;
}

HTTPResponse requestHandler(char *buffer)
{
  string req_message = string(buffer);

  HTTPRequest request = HTTPRequest(req_message);

  string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  if (!std::filesystem::exists(path))
  {
    return errorResponse("404", "Not found", "Couldn't find this file");
  }

  if (path.back() == '/' && std::filesystem::is_directory(path))
  {
    string parent_path = path.size() > ROOT_DIRECTORY.size() + 1
                             ? current_directory(
                                   path.substr(0, path.size() - 1))
                                   .erase(0, ROOT_DIRECTORY.size())
                             : "";

    std::cout << "parent_path " << parent_path << std::endl;

    std::string directory_template = readFile("./templates/directory.html");

    std::string list_html = "";
    if (parent_path.size())
      list_html += fmt::format("<li><a href=\"{0}\">{1}</a></li>", parent_path, "..");
    for (auto entry : std::filesystem::directory_iterator(path))
    {
      std::string filename = entry.path().string().erase(0, path.size());
      std::string filepath = filename + (entry.is_directory() ? "/" : "");
      list_html += fmt::format("<li><a href=\"./{0}\">{1}</a></li>", filepath, filename);
    }
    std::string directory_html = fmt::format(directory_template, request.URL, request.URL, list_html);

    HTTPResponse response = HTTPResponse();
    response.protocol = "HTTP/1.0";
    response.status_code = "200";
    response.status_phrase = "OK";

    response.headers["Connection"] = "close";
    response.headers["Content-type"] = "text/html";
    response.body = directory_html;
    response.headers["Content-Length"] = std::to_string(response.body.size());

    return response;
  }

  if (std::filesystem::is_regular_file(path))
  {
    HTTPResponse response = HTTPResponse();
    response.protocol = "HTTP/1.0";
    response.status_code = "200";
    response.status_phrase = "OK";

    response.headers["Connection"] = "close";
    response.headers["Content-type"] = getType(path);
    response.body = readFile(path);
    // response.body = "<html><head><title>example</title></head><body>Hello, world!</body></html>";
    response.headers["Content-Length"] = std::to_string(response.body.size());

    return response;
  }

  return errorResponse("404", "Not found", "Couldn't find this file");
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