#!/usr/bin/env python3
import os

# Get current working directory
cwd = os.getcwd()

# Try to access a file with relative path (in the same directory)
relative_file = "fortune.py"
file_exists = os.path.exists(relative_file)

# Try to read the first line of the file
file_content = ""
if file_exists:
    try:
        with open(relative_file, 'r') as f:
            file_content = f.readline().strip()
    except Exception as e:
        file_content = f"Error: {e}"

# List files in current directory
files = os.listdir(".")

body = f"""
<html>
<head><title>CGI Directory Test</title></head>
<body>
<h1>CGI Working Directory Test</h1>
<h2>Current Working Directory:</h2>
<pre>{cwd}</pre>

<h2>Relative file access test (fortune.py):</h2>
<p>File exists: <strong>{file_exists}</strong></p>
<p>First line: <code>{file_content}</code></p>

<h2>Files in current directory:</h2>
<ul>
{"".join(f"<li>{f}</li>" for f in sorted(files))}
</ul>
</body>
</html>
"""

print("HTTP/1.1 200 OK\r")
print("Content-Type: text/html\r")
print(f"Content-Length: {len(body.encode('utf-8'))}\r")
print("\r")
print(body)
