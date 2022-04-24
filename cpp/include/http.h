#ifndef SOCKET_HTTP
#define SOCKET_HTTP

#include <string>
#include <map>
#include "./urlencode.h"

class HTTPRequest
{
public:
  std::string method;
  std::string URL;
  std::map<std::string, std::string> url_params;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;

  HTTPRequest();
  HTTPRequest(std::string req_message);

  void clear();
  void parseRequestMessage(std::string req_message);
  std::map<std::string, std::string> parseBody();
  std::string toMessage();
};

class HTTPResponse
{
public:
  std::string protocol;
  std::string status_code;
  std::string status_phrase;
  std::map<std::string, std::string> headers;
  std::string body;

  HTTPResponse();
  HTTPResponse(std::string res_message);

  void clear();
  void parseResponseMessage(std::string res_message);
  std::string toMessage();
};

HTTPRequest::HTTPRequest()
{
  clear();
}

HTTPRequest::HTTPRequest(std::string req_message)
{
  clear();

  parseRequestMessage(req_message);
}

void HTTPRequest::clear()
{
  method = "";
  URL = "";
  url_params.clear();
  version = "";
  headers.clear();
  body = "";
}

void HTTPRequest::parseRequestMessage(std::string req_message)
{
  int j = 0;
  std::string url_param_key = "";
  for (int col = 0, url_param_flag = 0; j + 1 < req_message.size(); ++j)
  {
    if (req_message[j] == ' ')
    {
      ++col;
    }
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
        if (req_message[j] == '?' || req_message[j] == '&')
        {
          url_param_flag = 1;
          url_param_key = "";
        }
        else if (req_message[j] == '=')
        {
          url_param_flag = 2;
          url_params[url_param_key] = "";
        }
        else if (url_param_flag == 1)
          url_param_key += req_message[j];
        else if (url_param_flag == 2)
          url_params[url_param_key] += req_message[j];
        else
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
      ++j;
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

  URL = urlDecode(URL);
}

std::map<std::string, std::string> HTTPRequest::parseBody()
{
  std::map<std::string, std::string> parsed_body;
  const std::string FORM_URL_ENCODED = "application/x-www-form-urlencoded"; // ; charset=UTF-8

  std::string content_type = headers.find("Content-Type") == headers.end() ? FORM_URL_ENCODED : headers["Content-Type"];

  if (content_type.find(FORM_URL_ENCODED) != std::string::npos)
  {
    std::string key = "", value = "";
    for (int j = 0, flag = 0; j < body.size(); ++j)
    {
      if (body[j] == '=')
      {
        flag = 1;
      }
      else if (body[j] == '&')
      {
        parsed_body[urlDecode(key)] = urlDecode(value);
        key = "";
        value = "";
        flag = 0;
      }
      else
      {
        if (flag)
          value += body[j];
        else
          key += body[j];
      }
    }
    if (key.size() > 0 && value.size() > 0)
      parsed_body[urlDecode(key)] = urlDecode(value);
  }

  return parsed_body;
}

std::string HTTPRequest::toMessage()
{
  std::string url = URL;
  if (url_params.size())
    url += "&";
  for (auto itr = url_params.begin(); itr != url_params.end(); ++itr)
  {
    if (itr == url_params.begin())
      url += "?";
    else
      url += "&";
    url += itr->first + "=" + itr->second;
  }
  std::string request_line = method + " " + urlEncode(url) + " " + version + "\r\n";
  std::string header_lines = "";
  for (auto itr = headers.begin(); itr != headers.end(); ++itr)
  {
    header_lines += itr->first + ": " + itr->second + "\r\n";
  }
  return request_line + header_lines + "\r\n" + body;
}

HTTPResponse::HTTPResponse()
{
  clear();
}

HTTPResponse::HTTPResponse(std::string res_message)
{
  clear();

  parseResponseMessage(res_message);
}

void HTTPResponse::clear()
{
  protocol = "";
  status_code = "";
  status_phrase = "";
  headers.clear();
  body = "";
}

void HTTPResponse::parseResponseMessage(std::string res_message)
{
  int j = 0;
  for (int col = 0; j + 1 < res_message.size(); ++j)
  {
    if (col < 2 && res_message[j] == ' ')
    {
      ++col;
    }
    else if (res_message[j] == '\r' && res_message[j + 1] == '\n')
      break;
    else
    {
      switch (col)
      {
      case 0:
        protocol += res_message[j];
        break;
      case 1:
        status_code += res_message[j];
        break;
      case 2:
        status_phrase += res_message[j];
        break;
      }
    }
  }
  j += 2;
  std::string key;
  for (int col = 0; j + 1 < res_message.size(); ++j)
  {
    if (res_message[j] == ':' && res_message[j + 1] == ' ')
    {
      ++col;
      ++j;
      headers[key] = "";
    }
    else if (col && res_message[j] == '\r' && res_message[j + 1] == '\n')
    {
      col = 0;
      key = "";
      ++j;
    }
    else if (!col && res_message[j] == '\r' && res_message[j + 1] == '\n')
      break;
    else
    {
      switch (col)
      {
      case 0:
        key += res_message[j];
        break;
      case 1:
        headers[key] += res_message[j];
        break;
      }
    }
  }
  j += 2;
  for (; j < res_message.size(); ++j)
    body += res_message[j];
}

std::string HTTPResponse::toMessage()
{
  std::string status_line = protocol + " " + status_code + " " + status_phrase + "\r\n";
  std::string header_lines = "";
  for (auto itr = headers.begin(); itr != headers.end(); ++itr)
  {
    header_lines += itr->first + ": " + itr->second + "\r\n";
  }
  return status_line + header_lines + "\r\n" + body;
}

#endif