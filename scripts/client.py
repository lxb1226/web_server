import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '127.0.0.1'
port = 12345
s.connect((host, port))
msg = b'GET /welcome.html HTTP/1.1\r\n' \
      b'Host: www.baidu.com\r\n' \
      b'Connection: keep-alive\r\n' \
      b'\r\n'
s.send(msg)

res = s.recv(4096)
print(res.decode())
