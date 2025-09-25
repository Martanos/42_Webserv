#!/usr/bin/env python3

import subprocess
import time
import socket
import os
import signal

def test_webserv():
    print("=== Manual WebServ Test ===")
    
    # Change to project directory
    os.chdir("/home/malee/42_Projects/webserv")
    
    # Kill any existing processes
    try:
        subprocess.run(["pkill", "-f", "webserv"], check=False)
        time.sleep(1)
    except:
        pass
    
    # Start server
    print("Starting WebServ...")
    process = subprocess.Popen(["./webserv", "test.conf"], 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE,
                              text=True)
    
    # Wait for startup
    time.sleep(3)
    
    # Check if process is still running
    if process.poll() is not None:
        stdout, stderr = process.communicate()
        print("✗ Server failed to start")
        print(f"STDOUT: {stdout}")
        print(f"STDERR: {stderr}")
        return False
    
    print("✓ Server started successfully")
    
    # Test basic HTTP request
    print("\nTesting HTTP request...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 8080))
        
        request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        sock.send(request.encode())
        
        response = sock.recv(4096).decode()
        sock.close()
        
        print(f"Response received ({len(response)} bytes):")
        print(response[:200] + "..." if len(response) > 200 else response)
        
        if "HTTP/1.1" in response:
            print("✓ HTTP response received")
            return True
        else:
            print("✗ Invalid HTTP response")
            return False
            
    except Exception as e:
        print(f"✗ HTTP request failed: {e}")
        return False
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