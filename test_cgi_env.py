#!/usr/bin/env python3

import os
import sys

# CGI script that prints all environment variables
# This is useful for debugging and verifying CGI environment setup

print("Content-Type: text/html")
print()  # Empty line to separate headers from body

print("<html>")
print("<head>")
print("<title>CGI Environment Variables Test</title>")
print("<style>")
print("body { font-family: Arial, sans-serif; margin: 20px; }")
print("table { border-collapse: collapse; width: 100%; }")
print("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }")
print("th { background-color: #f2f2f2; }")
print("tr:nth-child(even) { background-color: #f9f9f9; }")
print(".env-name { font-weight: bold; color: #333; }")
print(".env-value { font-family: monospace; word-break: break-all; }")
print("</style>")
print("</head>")
print("<body>")

print("<h1>CGI Environment Variables</h1>")
print("<p>This page displays all environment variables available to the CGI script.</p>")

# Get all environment variables and sort them
env_vars = dict(os.environ)
sorted_vars = sorted(env_vars.items())

print("<table>")
print("<thead>")
print("<tr>")
print("<th>Variable Name</th>")
print("<th>Value</th>")
print("</tr>")
print("</thead>")
print("<tbody>")

for name, value in sorted_vars:
    # Escape HTML special characters
    name_escaped = name.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    value_escaped = value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    
    print("<tr>")
    print('<td class="env-name">{}</td>'.format(name_escaped))
    print('<td class="env-value">{}</td>'.format(value_escaped))
    print("</tr>")

print("</tbody>")
print("</table>")

print("<h2>Summary</h2>")
print("<p>Total environment variables: <strong>{}</strong></p>".format(len(env_vars)))

# Highlight important CGI variables
cgi_vars = [
    'REQUEST_METHOD', 'REQUEST_URI', 'QUERY_STRING', 'CONTENT_TYPE', 
    'CONTENT_LENGTH', 'HTTP_HOST', 'HTTP_USER_AGENT', 'HTTP_ACCEPT',
    'SERVER_NAME', 'SERVER_PORT', 'SERVER_PROTOCOL', 'SCRIPT_NAME',
    'PATH_INFO', 'REMOTE_ADDR', 'REMOTE_HOST', 'AUTH_TYPE', 'REMOTE_USER'
]

print("<h2>Important CGI Variables</h2>")
print("<table>")
print("<thead>")
print("<tr>")
print("<th>Variable Name</th>")
print("<th>Value</th>")
print("<th>Status</th>")
print("</tr>")
print("</thead>")
print("<tbody>")

for var in cgi_vars:
    value = env_vars.get(var, '')
    status = "Set" if value else "Not Set"
    status_color = "green" if value else "red"
    
    var_escaped = var.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    value_escaped = value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    
    print("<tr>")
    print('<td class="env-name">{}</td>'.format(var_escaped))
    print('<td class="env-value">{}</td>'.format(value_escaped))
    print('<td style="color: {}">{}</td>'.format(status_color, status))
    print("</tr>")

print("</tbody>")
print("</table>")

print("</body>")
print("</html>")

# Flush output to ensure all data is sent
sys.stdout.flush()
