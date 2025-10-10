<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "<h1>Login Successful</h1>";
    echo "<p>Welcome, " . htmlspecialchars($_POST['username']) . "!</p>";
    echo "<a href='index.php'>Back to Home</a>";
} else {
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Login</title>
</head>
<body>
    <h1>Login Form</h1>
    <form method="post">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required>
        <br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required>
        <br><br>
        <button type="submit">Login</button>
    </form>
    <p><a href="index.php">Back to Home</a></p>
</body>
</html>
<?php
}
?>
