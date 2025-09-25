#!/usr/bin/env python3

import subprocess
import time
import socket
import os
import signal
import sys

def run_command(cmd, timeout=10):
    """Run a command with timeout"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Command timed out"
    except Exception as e:
        return -1, "", str(e)

def test_server_startup():
    """Test if the server can start"""
    print("=== Testing Server Startup ===")
    
    # Kill any existing webserv processes
    run_command("pkill -f webserv")
    time.sleep(1)
    
    # Start server
    print("Starting WebServ...")
    process = subprocess.Popen(["./webserv", "test.conf"], 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE,
                              text=True)
    
    # Wait for startup
    time.sleep(3)
    
    # Check if process is still running
    if process.poll() is None:
        print("✓ Server started successfully")
        return process
    else:
        stdout, stderr = process.communicate()
        print("✗ Server failed to start")
        print(f"STDOUT: {stdout}")
        print(f"STDERR: {stderr}")
        return None

def test_http_request(port=8080, path="/"):
    """Test basic HTTP request"""
    print(f"=== Testing HTTP Request to localhost:{port}{path} ===")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', port))
        
        request = f"GET {path} HTTP/1.1\r\nHost: localhost\r\n\r\n"
        sock.send(request.encode())
        
        response = sock.recv(4096).decode()
        sock.close()
        
        print(f"Response received ({len(response)} bytes):")
        print(response[:300] + "..." if len(response) > 300 else response)
        
        if "HTTP/1.1" in response:
            print("✓ HTTP response received")
            return True
        else:
            print("✗ Invalid HTTP response")
            return False
            
    except Exception as e:
        print(f"✗ HTTP request failed: {e}")
        return False

def test_curl_request(port=8080, path="/"):
    """Test using curl"""
    print(f"=== Testing with curl localhost:{port}{path} ===")
    
    cmd = f"curl -s -w '%{{http_code}}' -o /tmp/curl_response http://localhost:{port}{path}"
    returncode, stdout, stderr = run_command(cmd, timeout=10)
    
    if returncode == 0:
        print(f"✓ Curl request successful")
        print(f"Response code: {stdout}")
        return True
    else:
        print(f"✗ Curl request failed: {stderr}")
        return False

def test_config_parsing():
    """Test configuration parsing"""
    print("=== Testing Configuration Parsing ===")
    
    # Test valid config
    returncode, stdout, stderr = run_command("./webserv test.conf", timeout=5)
    if returncode == 0:
        print("✓ Valid configuration accepted")
    else:
        print("✗ Valid configuration rejected")
        print(f"STDERR: {stderr}")
    
    # Test invalid config
    invalid_config = "invalid config content"
    with open("/tmp/invalid.conf", "w") as f:
        f.write(invalid_config)
    
    returncode, stdout, stderr = run_command("./webserv /tmp/invalid.conf", timeout=5)
    if returncode != 0:
        print("✓ Invalid configuration properly rejected")
    else:
        print("✗ Invalid configuration accepted")
    
    # Cleanup
    os.remove("/tmp/invalid.conf")

def main():
    print("WebServ Test Runner")
    print("==================")
    
    # Change to project directory
    os.chdir("/home/malee/42_Projects/webserv")
    
    # Test 1: Configuration parsing
    test_config_parsing()
    print()
    
    # Test 2: Server startup
    process = test_server_startup()
    if process is None:
        print("Cannot continue tests - server failed to start")
        return
    
    print()
    
    # Test 3: HTTP requests
    test_http_request(8080, "/")
    print()
    
    test_http_request(8080, "/index.html")
    print()
    
    test_http_request(8080, "/nonexistent.html")
    print()
    
    # Test 4: Curl requests
    test_curl_request(8080, "/")
    print()
    
    test_curl_request(8081, "/")
    print()
    
    # Cleanup
    print("=== Cleaning up ===")
    if process:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    
    print("Tests completed!")

if __name__ == "__main__":
    main()