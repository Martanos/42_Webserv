<?php
session_start();
if (!isset($_SESSION['user'])) {
    header("Location: login.php");
    exit();
}

if (isset($_GET['logout'])) {
    session_destroy();
    header("Location: login.php");
    exit();
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Showcase</title>
</head>
<body>
    <h2>Welcome, <?= htmlspecialchars($_SESSION['user']) ?></h2>
    <ul>
        <!-- <li><a href="login.php">Login Form</a></li> -->
        <li><a href="upload.php">File Upload</a></li>
        <li><a href="responses.php">Response Types</a></li>
        <li><a href="/Python/cgi_random.py">Python CGI Script</a></li>
        <li><a href="?logout=1">Logout</a></li>
    </ul>
</body>
</html>
