<?php
if (isset($_GET['error'])) {
    $code = (int)$_GET['error'];
    http_response_code($code);
    echo "<h1>Error $code</h1>";
    echo "<p>This should trigger the custom error page.</p>";
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Response Types</title>
</head>
<body>
    <h1>Test Response Types</h1>
    <p>Click to view custom error pages:</p>
    <ul>
        <li><a href="responses/400.php">Test 400 Bad Request Page</a></li>
        <li><a href="responses/403.php">Test 403 Forbidden Page</a></li>
        <li><a href="responses/500.php">Test 500 Internal Server Error Page</a></li>
    </ul>
    <p>Or test actual server errors:</p>
    <ul>
        <li><strong>404 Not Found:</strong> <a href="/nonexistent">Non-existent page</a></li>
        <li><strong>403 Forbidden:</strong> <a href="/Python/">Access Python directory</a></li>
    </ul>
    <p><a href="index.php">Back to Home</a></p>
</body>
