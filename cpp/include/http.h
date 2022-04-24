#ifndef SOCKET_HTTP
#define SOCKET_HTTP

#include <string>
#include <map>

std::string urlEncode(std::string str)
{
  std::string new_str = "";
  char c;
  int ic;
  const char *chars = str.c_str();
  char bufHex[10];
  int len = strlen(chars);

  for (int i = 0; i < len; i++)
  {
    c = chars[i];
    ic = c;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      new_str += c;
    else
    {
      sprintf(bufHex, "%X", c);
      if (ic < 16)
        new_str += "%0";
      else
        new_str += "%";
      new_str += bufHex;
    }
  }
  return new_str;
}

std::string urlDecode(std::string str)
{
  std::string ret;
  char ch;
  int i, ii, len = str.length();

  for (i = 0; i < len; i++)
  {
    if (str[i] != '%')
    {
      if (str[i] == '+')
        ret += ' ';
      else
        ret += str[i];
    }
    else
    {
      sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
      ch = static_cast<char>(ii);
      ret += ch;
      i = i + 2;
    }
  }
  return ret;
}

class HTTPRequest
{
public:
  std::string method;
  std::string URL;
  std::map<std::string, std::string> url_params;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;

  void clear()
  {
    method = "";
    URL = "";
    url_params.clear();
    version = "";
    headers.clear();
    body = "";
  }

  std::map<std::string, std::string> parseBody()
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

  HTTPRequest()
  {
    clear();
  }

  HTTPRequest(std::string req_message)
  {
    clear();

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

  std::string toMessage()
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
};

class HTTPResponse
{
public:
  std::string protocol;
  std::string status_code;
  std::string status_phrase;
  std::map<std::string, std::string> headers;
  std::string body;

  void clear()
  {
    protocol = "";
    status_code = "";
    status_phrase = "";
    headers.clear();
    body = "";
  }

  HTTPResponse() { clear(); }

  HTTPResponse(std::string res_message)
  {
    clear();

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