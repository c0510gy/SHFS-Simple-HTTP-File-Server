# socket-HTTP

## 개요

C++ socket programming을 이용하여 제작한 HTTP 기반의 파일 서버, 기본적인 디렉토리 탐색, 폴더 생성, 파일 제거, 파일 업로드 등의 기능 제공

## 개발환경

- C++20
- g++ Apple clang version 13.1.6 (clang-1316.0.21.2.3)
- Mac OS v12.3.1

## 사용

### 서버 컴파일

```
g++ -std=c++20 server.cpp -o server.o
```

### 클라이언트 컴파일

```
g++ -std=c++20 client.cpp -o client.o
```

## 서버 API

### GET \<path>

- \<path>가 `/`로 끝날 경우 해당 디렉토리 정보 반환
- \<path>가 `/`로 끝나지 않을 경우 해당 파일 반환

#### Request Example

```
GET / HTTP/1.0
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
Accept-Encoding: gzip, deflate
Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7

```

#### Response Structure

```
HTTP/1.0 200 OK
Connection: close
Content-Length: 0
Content-type: text/html

<디렉토리 정보 표현한 HTML>
```

```
HTTP/1.0 200 OK
Connection: close
Content-Length: <파일 크기>
Content-type: text/plain

<파일 내용>
```

### HEAD \<path>

- 주어진 \<path>에 대한 GET 메소드 요청의 반환 결과에서 header만 반환

#### Request Example

```
HEAD / HTTP/1.0
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
Accept-Encoding: gzip, deflate
Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7

```

#### Response Structure

```
HTTP/1.0 200 OK
Connection: close
Content-Length: 0
Content-type: text/html
```

### POST \<path>

- 주어진 디렉토리 \<path>에 새로운 디렉토리 생성

#### Body parameters

| Parameter        | Description               |
| ---------------- | ------------------------- |
| `directory_name` | 새로 생성할 디렉토리 이름 |

#### Request Example

```
POST / HTTP/1.0
Connection: keep-alive
Content-Length: 19
Accept: */*
X-Requested-With: XMLHttpRequest
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
Content-Type: application/x-www-form-urlencoded; charset=UTF-8
Accept-Encoding: gzip, deflate
Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7

directory_name=test
```

#### Response Structure

```
HTTP/1.0 200 OK
Connection: close
Content-Length: 0
```

### PUT \<path>

- 주어진 디렉토리 \<path>에 파일 업로드

#### Body parameters

| Parameter | Description            |
| --------- | ---------------------- |
| `file`    | base64로 인코딩된 파일 |
| `name`    | 저장될 파일 이름       |

#### Request Example

```
PUT / HTTP/1.0
Connection: keep-alive
Content-Length: 5554467
Accept: */*
X-Requested-With: XMLHttpRequest
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
Content-Type: application/x-www-form-urlencoded; charset=UTF-8
Accept-Encoding: gzip, deflate
Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7

file=JVBERi0xLjUKJY8KNSAwIG9iago8PCAvVHlwZSAvT2JqU3RtIC9GaWx0ZXIgL0ZsYXRlRGVjb2RlIC9GaXJzdCA4MTQgL0xlbmd0aCAxNjc0IC9OIDEwMCA%2BPgpzdHJlYW0KeNrNWF1P20oQffevmMf2AWe%2FvB8SqhSgYCR6W5Xq8tDyYBIXrCZ25Dht6a%2B%2FZ%2BI1AS4Q2qvqVm2YtXfn7JmZs2uvDQnypAUFMo6kIoe%2FGo2MpCWZwThSQuJHSsPgv1WkDKmgEozSAv1A0DCBtMVPkQ4wmgwgtCUDZHQbi58gEzw6KAOYMZRZlZiMsoArT1Yp9JN1uAMqwmIYOcvDyAXcdOQxpwVn%2FKykIHBlKBiR2IyCx5UnKUAWRKRAME6RlBwXgpKY1yEqiT8cpwIrL2AxyEvEjQm9gTUyAZTULpAHnpGYEHjGIU%2FAyzBrAF6G4AJnyYEJ8DgKKQ...<생략>&name=test.pdf
```

#### Response Structure

```
HTTP/1.0 200 OK
Connection: close
Content-Length: 0
```

### DELETE \<path>

- 주어진 디렉토리 \<path>에 있는 파일 또는 디렉토리 삭제

#### Body parameters

| Parameter   | Description                    |
| ----------- | ------------------------------ |
| `file_name` | 삭제할 파일 또는 디렉토리 이름 |

#### Request Example

```
DELETE / HTTP/1.0
Connection: keep-alive
Content-Length: 14
Accept: */*
X-Requested-With: XMLHttpRequest
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36
Content-Type: application/x-www-form-urlencoded; charset=UTF-8
Accept-Encoding: gzip, deflate
Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7

file_name=test
```

#### Response Structure

```
HTTP/1.0 200 OK
Connection: close
Content-Length: 0
```

##
