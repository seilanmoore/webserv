#!/usr/bin/env python3
import socket
import sys

HOST = 'localhost'
PORT = 8080

def send_request(path):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))
        request = f"GET {path} HTTP/1.1\r\nHost: {HOST}:{PORT}\r\nConnection: close\r\n\r\n"
        sock.send(request.encode())
        
        response = b""
        while True:
            data = sock.recv(4096)
            if not data:
                break
            response += data
        sock.close()
        return response.decode(errors='ignore')
    except Exception as e:
        print(f"Error connecting to server: {e}")
        return None

def parse_status(response):
    if not response:
        return 0, "No response"
    lines = response.split('\r\n')
    status_line = lines[0]
    parts = status_line.split(' ')
    if len(parts) >= 2:
        return int(parts[1]), status_line
    return 0, status_line

def test_missing_cgi():
    print("Testing functionality: Requesting non-existent CGI script...")
    path = "/cgi-bin/this_script_does_not_exist.py"
    response = send_request(path)
    code, status_line = parse_status(response)
    
    print(f"Request: GET {path}")
    print(f"Response Status: {status_line}")
    
    if code == 404:
        print("✅ SUCCESS: Server returned 404 Not Found")
        return True
    else:
        print(f"❌ FAILURE: Expected 404, got {code}")
        return False

if __name__ == "__main__":
    if test_missing_cgi():
        sys.exit(0)
    else:
        sys.exit(1)
