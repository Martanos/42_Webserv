#!/usr/bin/php
<?php
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

// Parse query string
function get_query_param($name) {
    $query = $_SERVER['QUERY_STRING'] ?? '';
    parse_str($query, $params);
    return $params[$name] ?? null;
}

$auth_user = get_cookie('auth');

if (!$auth_user) {
    echo "Status: 302 Found\r\n";
    echo "Location: login.php\r\n";
    echo "\r\n";
    exit();
}

if (get_query_param('logout')) {
    // Clear cookie by setting expired date
    echo "Status: 302 Found\r\n";
    echo "Set-Cookie: auth=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n";
    echo "Location: login.php\r\n";
    echo "\r\n";
    exit();
}

echo "Content-Type: text/html\r\n";
echo "\r\n";
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Showcase</title>
</head>
<body>
    <h2>Welcome, <?= htmlspecialchars($auth_user) ?></h2>
    <ul>
        <li><a href="file_upload.php">File Upload</a></li>
        <li><a href="upload.php">File Upload (alt)</a></li>
        <li><a href="responses.php">Response Types</a></li>
        <li><a href="/Python/cgi_random.py">Python CGI Script</a></li>
        <li><a href="?logout=1">Logout</a></li>
    </ul>
</body>
</html>
