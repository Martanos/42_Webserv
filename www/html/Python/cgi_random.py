#!/usr/bin/env python3

import random
import cgi

print("Content-type: text/html\n\n")

print("""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Random CGI</title>
</head>
<body>
    <h1>Random Number Generator</h1>
    <p>Random number (1-100): <strong>{}</strong></p>
    <p>Dice roll (1-6): <strong>{}</strong></p>
    <p><a href="/index.php">Back to Home</a></p>
</body>
</html>
""".format(random.randint(1, 100), random.randint(1, 6)))
