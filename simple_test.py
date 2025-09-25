#!/usr/bin/env python3

import socket
import time
import subprocess
import os
import signal

def test_webserv():
    print("=== WebServ Simple Test ===")
    
    # Start the server
    print("Starting WebServ...")
    process = subprocess.Popen(["./webserv", "test.conf"], 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE)
    
    # Wait for server to start
    time.sleep(3)
    
    try:
        # Test basic HTTP request
        print("Testing basic HTTP request...")
        
        # Create socket connection
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        
        try:
            sock.connect(('localhost', 8080))
            
            # Send HTTP request
            request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
            sock.send(request.encode())
            
            # Receive response
            response = sock.recv(4096).decode()
            print("Response received:")
            print(response[:200] + "..." if len(response) > 200 else response)
            
            if "HTTP/1.1" in response:
                print("✓ Server responds to HTTP requests")
            else:
                print("✗ Server does not respond properly")
                
        except Exception as e:
            print(f"✗ Connection failed: {e}")
        finally:
            sock.close()
            
        # Test 404 error
        print("\nTesting 404 error...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        
        try:
            sock.connect(('localhost', 8080))
            request = "GET /nonexistent.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
            sock.send(request.encode())
            response = sock.recv(4096).decode()
            
            if "404" in response:
                print("✓ 404 error handling works")
            else:
                print("✗ 404 error handling failed")
                
        except Exception as e:
            print(f"✗ 404 test failed: {e}")
        finally:
            sock.close()
            
    finally:
        # Clean up
        print("\nStopping server...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

if __name__ == "__main__":
    test_webserv()