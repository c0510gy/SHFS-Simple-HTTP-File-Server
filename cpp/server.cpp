#include <iostream>
#include <string>
#include <map>
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
#define PORT 1234

const std::string ROOT_DIRECTORY = "./root";

using std::cerr;
using std::map;
using std::string;

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

HTTPResponse getHanlder(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  if (!std::filesystem::exists(path))
  {
    return errorResponse("404", "Not found", "Couldn't find this file");
  }

  if (path.back() == '/' && std::filesystem::is_directory(path))
  {
    string parent_path = path.size() > ROOT_DIRECTORY.size() + 1
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

HTTPResponse postHanlder(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  std::map<std::string, std::string> parsed_body = request.parseBody();

  std::string directory_name = parsed_body["directory_name"];

  if (directory_name.size() && !std::filesystem::exists(path + directory_name))
  {
    std::filesystem::create_directory(path + directory_name);
  }

  return getHanlder(request);
}

HTTPResponse deleteHanlder(HTTPRequest &request)
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

  return getHanlder(request);
}

HTTPResponse putHanlder(HTTPRequest &request)
{
  std::string path = ROOT_DIRECTORY + parsePath(request.URL);
  std::cout << path << std::endl;

  std::map<std::string, std::string> parsed_body = request.parseBody();

  std::cout << "new_file_name " << parsed_body["name"] << std::endl;

  std::string file = base64_decode(parsed_body["file"]);

  std::ofstream ofs(path + parsed_body["name"], std::ios::binary);
  ofs.write(file.c_str(), file.size());
  ofs.close();

  return getHanlder(request);
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
  */
  if (request.method == "GET")
  {
    return getHanlder(request);
  }
  else if (request.method == "POST")
  {
    return postHanlder(request);
  }
  else if (request.method == "DELETE")
  {
    return deleteHanlder(request);
  }
  else if (request.method == "PUT")
  {
    return putHanlder(request);
  }

  return errorResponse("501", "Not Implemented", "Couldn't handle this method");
}

HTTPResponse requestHandler(char *buffer)
{
  HTTPResponse response;
  try
  {
    string req_message = string(buffer);

    HTTPRequest request = HTTPRequest(req_message);

    response = methodHandler(request);
  }
  catch (int expn)
  {
    response = errorResponse("501", "Not Implemented", "Couldn't handle this method");
  }

  return response;
}

int main()
{
  int server_fd, listen_fd, valread;
  int opt = 1;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[1048576 * 7] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    cerr << "socket failed";
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("192.168.0.10"); // INADDR_ANY;
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
      std::cout << "listen";
      return -1;
    }

    if ((listen_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      std::cout << "accept";
      return -1;
    }

    int pid = fork();

    if (pid != 0)
    {
      continue;
    }

    memset(buffer, 0, sizeof(buffer));
    char *buffer_ptr = buffer;
    char *buffer_end = buffer + sizeof(buffer);
    int content_length = -1;
    int received = 0;
    do
    {
      char tmp_buffer[1024] = {0};
      memset(tmp_buffer, 0, sizeof(tmp_buffer));
      valread = read(listen_fd, tmp_buffer, 1024);

      if (valread > 0)
      {
        if (!((buffer_ptr + valread) <= buffer_end))
          break;

        int header_end = -1;
        for (int j = 0; j + 3 < valread; ++j)
        {
          if (tmp_buffer[j] == '\r' && tmp_buffer[j + 1] == '\n' && tmp_buffer[j + 2] == '\r' && tmp_buffer[j + 3] == '\n')
          {
            header_end = j;
            break;
          }
        }
        if (header_end != -1)
        {
          memcpy(buffer_ptr, tmp_buffer, header_end);
          HTTPRequest request_header = HTTPRequest(buffer);
          if (request_header.headers.find("Content-Length") != request_header.headers.end())
          {
            content_length = atoi(request_header.headers["Content-Length"].c_str());
          }
        }

        received += valread;
        fprintf(stderr, "Bytes received: %d\t total: %d\t content-length: %d\n", valread, received, content_length);

        memcpy(buffer_ptr, tmp_buffer, valread);
        buffer_ptr += valread;
      }
      else if (valread == 0)
      {
        break;
      }
      else
      {
        // fprintf(stderr, "recv failed: ");
        break;
      }
    } while ((valread == 1024 && buffer_ptr < buffer_end) || (content_length != -1 && (buffer_ptr - buffer + 1) < content_length)); // check for end of buffer

    printf("%.1024s\n", buffer);
    HTTPResponse response = requestHandler(buffer);
    string response_message = response.toMessage();

    int bufferSize = 1024;
    int messageLength = response_message.size();
    int sendPosition = 0;

    char *response_message_ptr = response_message.data();
    char sending_buffer[1024] = {0};
    memset(sending_buffer, 0, sizeof(sending_buffer));

    while (messageLength)
    {
      int chunkSize = messageLength > bufferSize ? bufferSize : messageLength;
      memcpy(sending_buffer, response_message_ptr + sendPosition, chunkSize);
      chunkSize = send(listen_fd, sending_buffer, chunkSize, NULL);
      messageLength -= chunkSize;
      sendPosition += chunkSize;
    }

    close(listen_fd);
    return 0;
  }

  return 0;
}