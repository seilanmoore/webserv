import os
import sys
from urllib.parse import parse_qs

method = os.environ.get("REQUEST_METHOD", "GET")

params = {}
post_body = ""
if method == "GET":
    query_string = os.environ.get("QUERY_STRING", "")
    params = parse_qs(query_string)
elif method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    post_body = sys.stdin.read(content_length) if content_length > 0 else ""
    params = parse_qs(post_body)

name = params.get("name", ["Unknown"])[0].upper()
message = params.get("message", ["Unknown"])[0].upper()

print("Content-Type: text/html")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Form Result</title></head>")
print("<body>")
print("<h1>Data Received</h1>")
print(f'<p style="color: red; font-size: 24px; font-weight: bold;">Name: {name}</p>')
print(f'<p style="color: red; font-size: 24px; font-weight: bold;">Message: {message}</p>')
if post_body != "":
  print(f'<p style="color: red; font-size: 24px; font-weight: bold;">Raw Body: {post_body}</p>')
print("</body>")
print("</html>")
