#!/usr/bin/env python3
"""Test internal redirect (local path, status 200)"""

print("Location: /index.html")
print()
# Status defaults to 200, Location starts with /, no scheme
# Should trigger internal redirect
