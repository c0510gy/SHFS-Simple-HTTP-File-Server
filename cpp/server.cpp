#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "./include/http.h"
#include "./include/base64.h"
#define FMT_HEADER_ONLY
#include "./lib/fmt/core.h"

#define ADDRESS "192.168.0.10"
#define PORT 1234
#define MAX_HEADER_SIZE 8192
#define MAX_RECV_SIZE 8192
#define MAX_SEND_SIZE 8192
#define MAX_CONNECTION 10

const std::string ROOT_DIRECTORY = "./root";

std::string currentDirectory(std::string path);
std::string readFile(std::string filepath);
std::string getType(std::string filepath);
std::string parsePath(std::string url);

HTTPResponse requestHandler(char *buffer);
HTTPResponse methodHandler(HTTPRequest &request);
HTTPResponse getHandler(HTTPRequest &request);
HTTPResponse headHandler(HTTPRequest &request);
HTTPResponse postHandler(HTTPRequest &request);
HTTPResponse deleteHandler(HTTPRequest &request);
HTTPResponse putHandler(HTTPRequest &request);
HTTPResponse errorResponse(std::string error_code, std::string error_phrase, std::string error_message);

int main()
{
  int server_fd, listen_fd, valread;
  int opt = 1;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[MAX_HEADER_SIZE] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    std::cerr << "socket failed";
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ADDRESS); // INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    std::cerr << "bind failed";
    return -1;
  }

  while (true)
  {
    if (listen(server_fd, MAX_CONNECTION) < 0)
    {
      std::cerr << "listen";
      return -1;
    }

    if ((listen_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      std::cerr << "accept";
      return -1;
    }

    int pid = fork();

    if (pid != 0)
    {
      continue;
    }

    memset(buffer, 0, sizeof(buffer));
    std::vector<char> recv_buffer(MAX_HEADER_SIZE, 0);

    int content_length = -1;
    valread = read(listen_fd, buffer, MAX_HEADER_SIZE);
    if (valread > 0)
    {
      int header_end = -1;
      for (int j = 0; j + 3 < valread; ++j)
      {
        if (buffer[j] == '\r' && buffer[j + 1] == '\n' && buffer[j + 2] == '\r' && buffer[j + 3] == '\n')
        {
          header_end = j;
          break;
        }
      }
      if (header_end == -1)
      {
        // To do
        errorResponse("413", "Entity Too Large", "Exceeded maximum header size");
      }
      else
      {
        memcpy(recv_buffer.data(), buffer, header_end);
        HTTPRequest request_header = HTTPRequest(recv_buffer.data());
        if (request_header.headers.find("Content-Length") != request_header.headers.end())
        {
          content_length = atoi(request_header.headers["Content-Length"].c_str());
        }

        memcpy(recv_buffer.data(), buffer, valread);
      }
    }

    if (content_length > 0)
    {
      recv_buffer.resize(content_length);

      for (int received = valread; received < content_length; received += valread)
      {
        valread = read(listen_fd, buffer, MAX_RECV_SIZE);
        if (valread > 0)
        {
          fprintf(stderr, "Bytes received: %d\t total: %d\t content-length: %d\n", valread, received + valread, content_length);
          memcpy(recv_buffer.data() + received, buffer, valread);
        }
      }
    }
    printf("%.1024s\n", recv_buffer.data());

    HTTPResponse response = requestHandler(recv_buffer.data());
    std::string response_message = response.toMessage();

    int buffer_size = MAX_SEND_SIZE;
    char sending_buffer[MAX_SEND_SIZE] = {0};
    memset(sending_buffer, 0, sizeof(sending_buffer));
    for (int sent = 0, chunk_size; sent < response_message.size(); sent += chunk_size)
    {
      chunk_size = (int)response_message.size() - sent > buffer_size ? buffer_size : (int)response_message.size() - sent;
      memcpy(sending_buffer, response_message.data() + sent, chunk_size);
      chunk_size = send(listen_fd, sending_buffer, chunk_size, 0);
      fprintf(stderr, "Bytes sent: %d\t total: %d\t content-length: %d\n", chunk_size, sent + chunk_size, (int)response_message.size());
    }

    close(listen_fd);
    return 0;
  }

  return 0;
}

HTTPResponse requestHandler(char *buffer)
{
  HTTPResponse response;
  try
  {
    std::string req_message = std::string(buffer);

    HTTPRequest request = HTTPRequest(req_message);

    response = methodHandler(request);
  }
  catch (int expn)
  {
    response = errorResponse("501", "Not Implemented", "Couldn't handle this method");
  }

  return response;
}

HTTPResponse methodHandler(HTTPRequest &request)
{
  /*
  GET URL
    URL이 가리키는 디렉토리 조회 또는 파일 가져오기
  POST
    디렉토리 생성
  PUT
    파일 업로드
  DELETE
    파일 또는 디렉토리 삭제
  HEAD
    GET without body
  */
  if (request.method == "GET")
  {
    return getHandler(request);
  }
  else if (request.method == "HEAD")
  {
    return headHandler(request);
  }
  else if (request.method == "POST")
  {
    return postHandler(request);
  }
  else if (request.method == "DELETE")
  {
    return deleteHandler(request);
  }
  else if (request.method == "PUT")
  {
    return putHandler(request);
  }

  return errorResponse("501", "Not Implemented", "Couldn't handle this method");
}

HTTPResponse getHandler(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  if (!std::filesystem::exists(path))
  {
    return errorResponse("404", "Not found", "Couldn't find this file");
  }

  if (path.back() == '/' && std::filesystem::is_directory(path))
  {
    std::string parent_path = path.size() > ROOT_DIRECTORY.size() + 1
                                  ? currentDirectory(
                                        path.substr(0, path.size() - 1))
                                        .erase(0, ROOT_DIRECTORY.size())
                                  : "";

    std::cout << "parent_path " << parent_path << std::endl;

    std::string directory_template = readFile("./templates/directory.html");

    std::string list_html = "";
    if (parent_path.size())
      list_html += fmt::format("<tr><td colspan='4'><a href=\"{0}\">{1}</a></td></tr>", parent_path, "..");

    for (auto entry : std::filesystem::directory_iterator(path))
    {
      std::string filename = entry.path().string().erase(0, path.size());
      std::string filepath = filename + (entry.is_directory() ? "/" : "");

      const std::string FILE_ICON = "<i class='fa-solid fa-file'></i>";
      const std::string DIRECTORY_ICON = "<i class='fa-solid fa-folder'></i>";

      if (entry.is_regular_file())
      {
        list_html += fmt::format("<tr><td><a style='display:inline-block;white-space: nowrap;overflow: hidden;text-overflow: ellipsis;max-width: 20ch;' href=\"./{0}\">{2} {1}</a></td>", filepath, filename, FILE_ICON);
        list_html += fmt::format(std::locale("en_US.UTF-8"), "<td>{0:L}</td>", entry.file_size());
      }
      else
      {
        list_html += fmt::format("<tr><td><a style='display:inline-block;white-space: nowrap;overflow: hidden;text-overflow: ellipsis;max-width: 20ch;' href=\"./{0}\">{2} {1}</a></td>", filepath, filename, DIRECTORY_ICON);
        list_html += "<td></td>";
      }
      std::time_t cftime = decltype(entry.last_write_time())::clock::to_time_t(entry.last_write_time());
      list_html += fmt::format("<td>{0}</td>", std::asctime(std::localtime(&cftime)));
      list_html += fmt::format("<td><button onclick=\"delete_('{0}')\">delete</button></td>", filename);
      list_html += "</tr>";
    }
    std::string directory_html = directory_template;
    directory_html = std::regex_replace(directory_html, std::regex("\\{\\{title\\}\\}"), request.URL);
    directory_html = std::regex_replace(directory_html, std::regex("\\{\\{header\\}\\}"), request.URL);
    directory_html = std::regex_replace(directory_html, std::regex("\\{\\{content\\}\\}"), list_html);

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
    response.headers["Content-Length"] = std::to_string(response.body.size());

    return response;
  }

  return errorResponse("403", "Forbidden", "Couldn't read the file");
}

HTTPResponse headHandler(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  HTTPResponse response = getHandler(request);
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";
  response.headers["Connection"] = "close";
  response.body = "";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
}

HTTPResponse postHandler(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  std::map<std::string, std::string> parsed_body = request.parseBody();

  std::string directory_name = parsed_body["directory_name"];

  if (directory_name.size() && !std::filesystem::exists(path + directory_name))
  {
    std::filesystem::create_directory(path + directory_name);
  }

  HTTPResponse response = HTTPResponse();
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";
  response.headers["Connection"] = "close";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
}

HTTPResponse deleteHandler(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  std::map<std::string, std::string> parsed_body = request.parseBody();

  std::string file_name = parsed_body["file_name"];

  std::cout << "file_name " << file_name << std::endl;
  if (file_name.size() && std::filesystem::exists(path + file_name))
  {
    std::filesystem::remove_all(path + file_name);
  }

  HTTPResponse response = HTTPResponse();
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";
  response.headers["Connection"] = "close";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
}

HTTPResponse putHandler(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  std::map<std::string, std::string> parsed_body = request.parseBody();

  std::cout << "new_file_name " << parsed_body["name"] << std::endl;

  std::string file = base64Decode(parsed_body["file"]);

  std::ofstream ofs(path + parsed_body["name"], std::ios::binary);
  ofs.write(file.c_str(), file.size());
  ofs.close();

  HTTPResponse response = HTTPResponse();
  response.protocol = "HTTP/1.0";
  response.status_code = "200";
  response.status_phrase = "OK";
  response.headers["Connection"] = "close";
  response.headers["Content-Length"] = std::to_string(response.body.size());

  return response;
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

std::string currentDirectory(std::string path)
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
    return "video/mp4";
  else if (filepath.find(".mov") != std::string::npos)
    return "video/mp4";
  else if (filepath.find(".pdf") != std::string::npos)
    return "application/pdf";

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
