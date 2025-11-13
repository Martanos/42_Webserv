#!/usr/bin/env python3
import os
print("Status: 200 OK")
print("Content-Type: text/plain\n")
print("Hello from CGI test")
for key in sorted(os.environ):
    if key.startswith("HTTP_"):
        print(f"{key}={os.environ[key]}")
