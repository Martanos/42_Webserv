#!/usr/bin/env python3
"""Basic CGI test - returns normal document response"""

print("Content-Type: text/html")
print("Status: 200 OK")
print()
print("<html><body>")
print("<h1>Basic CGI Test</h1>")
print("<p>This is a normal document response.</p>")
print("</body></html>")
