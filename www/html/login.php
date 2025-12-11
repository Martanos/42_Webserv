#!/usr/bin/php
<?php
// Parse POST data from stdin for CGI mode
function parse_post_data() {
    $content_type = $_SERVER['CONTENT_TYPE'] ?? '';

    // Handle application/x-www-form-urlencoded
    if (strpos($content_type, 'application/x-www-form-urlencoded') !== false ||
        $_SERVER['REQUEST_METHOD'] === 'POST') {
        $raw_data = file_get_contents('php://stdin');
        parse_str($raw_data, $post_data);
        return $post_data;
    }
    return [];
}

// Parse cookies from HTTP_COOKIE
function get_cookie($name) {
    $cookies = [];
    if (isset($_SERVER['HTTP_COOKIE'])) {
        $cookie_parts = explode(';', $_SERVER['HTTP_COOKIE']);
        foreach ($cookie_parts as $part) {
            $part = trim($part);
            if (strpos($part, '=') !== false) {
                list($key, $value) = explode('=', $part, 2);
                $cookies[trim($key)] = trim($value);
            }
        }
    }
    return $cookies[$name] ?? null;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $post = parse_post_data();
    $username = $post['username'] ?? '';
    $password = $post['password'] ?? '';

    if ($username === 'admin' && $password === '1234') {
        // Set a simple auth cookie (in production, use secure tokens!)
        echo "Status: 302 Found\r\n";
        echo "Set-Cookie: auth=admin; Path=/\r\n";
        echo "Location: index.php\r\n";
        echo "\r\n";
        exit();
    } else {
        $error = "Invalid login.";
    }
}

echo "Content-Type: text/html\r\n";
echo "\r\n";
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Login</title>
</head>
<body>
    <h3>Login Form</h3>
    <form method="post">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required>
        <br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required>
        <br><br>
        <button type="submit">Login</button>
    </form>
    <?php if (isset($error)) echo "<p style='color:red;'>$error</p>"; ?>
</body>
</html>
