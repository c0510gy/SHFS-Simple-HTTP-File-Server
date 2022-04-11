#ifndef SOCKET_HTTP
#define SOCKET_HTTP

#include <string>
#include <map>

class HTTPRequest
{
public:
  std::string method;
  std::string URL;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;

  HTTPRequest() {}
  HTTPRequest(std::string req_message)
  {
    int j = 0;
    for (int col = 0; j + 1 < req_message.size(); ++j)
    {
      if (req_message[j] == ' ')
        ++col;
      else if (req_message[j] == '\r' && req_message[j + 1] == '\n')
        break;
      else
      {
        switch (col)
        {
        case 0:
          method += req_message[j];
          break;
        case 1:
          URL += req_message[j];
          break;
        case 2:
          version += req_message[j];
          break;
        }
      }
    }
    j += 2;
    std::string key;
    for (int col = 0; j + 1 < req_message.size(); ++j)
    {
      if (req_message[j] == ':' && req_message[j + 1] == ' ')
      {
        ++col;
        headers[key] = "";
      }
      else if (col && req_message[j] == '\r' && req_message[j + 1] == '\n')
      {
        col = 0;
        key = "";
        ++j;
      }
      else if (!col && req_message[j] == '\r' && req_message[j + 1] == '\n')
        break;
      else
      {
        switch (col)
        {
        case 0:
          key += req_message[j];
          break;
        case 1:
          headers[key] += req_message[j];
          break;
        }
      }
    }
    j += 2;
    for (; j < req_message.size(); ++j)
      body += req_message[j];
  }
};

class HTTPResponse
{
public:
  std::string protocol;
  std::string status_code;
  std::string status_phrase;
  std::map<std::string, std::string> headers;
  std::string body;

  HTTPResponse() {}

  std::string toMessage()
  {
    std::string status_line = protocol + " " + status_code + " " + status_phrase + "\r\n";
    std::string header_lines = "";
    for (auto itr = headers.begin(); itr != headers.end(); ++itr)
    {
      header_lines += itr->first + ": " + itr->second + "\r\n";
    }
    return status_line + header_lines + "\r\n" + body;
  }
};

#endif