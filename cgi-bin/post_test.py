#!/usr/bin/env python3
import sys
import os

# Read POST body from stdin
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_body = sys.stdin.read(content_length) if content_length > 0 else ""

# Output CGI headers
print("Content-Type: text/html")
print("")

# Output HTML
print("<html><head><title>POST Test</title></head>")
print("<body style='display:flex; flex-direction:column; align-items:center; justify-content:center; min-height:100vh;'>")
print("<div style='text-align:center;'>")
print("<h1>POST Data Received</h1>")
print(f"<p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'N/A')}</p>")
print(f"<p><strong>Content-Type:</strong> {os.environ.get('CONTENT_TYPE', 'N/A')}</p>")
print(f"<p><strong>Content-Length:</strong> {os.environ.get('CONTENT_LENGTH', 'N/A')}</p>")
print(f"<p><strong>Query String:</strong> {os.environ.get('QUERY_STRING', 'N/A')}</p>")
print(f"<p><strong>POST Body:</strong></p>")
print(f"<pre style='background:#eee;padding:10px;'>{post_body if post_body else '(empty)'}</pre>")
print("</div></body></html>")
