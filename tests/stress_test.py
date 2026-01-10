#!/usr/bin/env python3
"""
Stress test script for webserv
Tests concurrent connections, various HTTP methods, and error handling
"""

import socket
import threading
import time
import sys

HOST = 'localhost'
PORT = 8080
NUM_THREADS = 50
REQUESTS_PER_THREAD = 20

results = {
    'success': 0,
    'failed': 0,
    'timeout': 0
}
lock = threading.Lock()

def make_request(method, path, body=None):
    """Make a simple HTTP request and return status code"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect((HOST, PORT))
        
        request = f"{method} {path} HTTP/1.1\r\n"
        request += f"Host: {HOST}:{PORT}\r\n"
        
        if body:
            request += f"Content-Length: {len(body)}\r\n"
            request += "Content-Type: application/x-www-form-urlencoded\r\n"
        
        request += "Connection: close\r\n"
        request += "\r\n"
        
        if body:
            request += body
        
        sock.send(request.encode())
        
        response = b""
        while True:
            try:
                data = sock.recv(4096)
                if not data:
                    break
                response += data
            except socket.timeout:
                break
        
        sock.close()
        
        # Parse status code
        if response:
            first_line = response.decode('utf-8', errors='ignore').split('\r\n')[0]
            if 'HTTP/1.1' in first_line:
                return int(first_line.split()[1])
        return 0
        
    except socket.timeout:
        return -1
    except Exception as e:
        return -2

def worker(thread_id):
    """Worker thread that makes multiple requests"""
    global results
    
    tests = [
        ('GET', '/', None),
        ('GET', '/about', None),
        ('GET', '/noexiste', None),  # Should be 404
        ('POST', '/upload/test.txt', 'test data'),
        ('GET', '/cgi-bin/fortune.py', None),
    ]
    
    for i in range(REQUESTS_PER_THREAD):
        test = tests[i % len(tests)]
        status = make_request(test[0], test[1], test[2])
        
        with lock:
            if status == -1:
                results['timeout'] += 1
            elif status < 0 or status >= 500:
                results['failed'] += 1
            else:
                results['success'] += 1

def run_stress_test():
    print(f"Starting stress test: {NUM_THREADS} threads, {REQUESTS_PER_THREAD} requests each")
    print(f"Total requests: {NUM_THREADS * REQUESTS_PER_THREAD}")
    print("-" * 50)
    
    start_time = time.time()
    
    threads = []
    for i in range(NUM_THREADS):
        t = threading.Thread(target=worker, args=(i,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
    
    elapsed = time.time() - start_time
    total = results['success'] + results['failed'] + results['timeout']
    
    print("-" * 50)
    print(f"Completed in {elapsed:.2f} seconds")
    print(f"Requests per second: {total / elapsed:.2f}")
    print(f"Success: {results['success']} ({100*results['success']/total:.1f}%)")
    print(f"Failed: {results['failed']} ({100*results['failed']/total:.1f}%)")
    print(f"Timeout: {results['timeout']} ({100*results['timeout']/total:.1f}%)")
    
    if results['success'] / total >= 0.95:
        print("\n✅ STRESS TEST PASSED")
        return 0
    else:
        print("\n❌ STRESS TEST FAILED")
        return 1

if __name__ == '__main__':
    sys.exit(run_stress_test())
