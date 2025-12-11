#!/usr/bin/php
<?php
$upload_dir = __DIR__ . '/uploads/';
$message = '';

// Ensure upload directory exists
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0755, true);
}

// Parse multipart form data from stdin for CGI
function parse_multipart_stdin() {
    $content_type = $_SERVER['CONTENT_TYPE'] ?? '';
    if (strpos($content_type, 'multipart/form-data') === false) {
        return null;
    }

    // Get boundary from Content-Type
    preg_match('/boundary=(.*)$/', $content_type, $matches);
    if (empty($matches[1])) {
        return null;
    }
    $boundary = $matches[1];

    // Read stdin
    $raw_data = file_get_contents('php://stdin');
    if (empty($raw_data)) {
        return null;
    }

    // Split by boundary
    $parts = explode('--' . $boundary, $raw_data);

    foreach ($parts as $part) {
        if (empty($part) || $part == "--\r\n") continue;

        // Check if this is the file part
        if (preg_match('/Content-Disposition: form-data; name="file"; filename="([^"]+)"/', $part, $matches)) {
            $filename = $matches[1];
            // Get content after the double CRLF
            $content_start = strpos($part, "\r\n\r\n");
            if ($content_start === false) continue;
            $content = substr($part, $content_start + 4);
            // Remove trailing CRLF
            $content = rtrim($content, "\r\n");

            return ['filename' => $filename, 'content' => $content];
        }
    }
    return null;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $file_data = parse_multipart_stdin();
    if ($file_data && !empty($file_data['filename'])) {
        $target = $upload_dir . basename($file_data['filename']);
        if (file_put_contents($target, $file_data['content']) !== false) {
            $message = "<h1>Upload Successful</h1>";
            $message .= "<p>File uploaded: " . htmlspecialchars(basename($file_data['filename'])) . "</p>";
        } else {
            $message = "<h1>Upload Failed</h1><p>Error saving file.</p>";
        }
    } else {
        $message = "<h1>Upload Failed</h1><p>No file received or invalid format.</p>";
    }
}

// Handle file deletion
if ($_SERVER['REQUEST_METHOD'] === 'DELETE' || (isset($_GET['action']) && $_GET['action'] === 'delete' && isset($_GET['file']))) {
    $filename = $_GET['file'];
    $filepath = $upload_dir . $filename;

    if (file_exists($filepath)) {
        if (unlink($filepath)) {
            $message = "<h1>Delete Successful</h1><p>File '$filename' has been deleted.</p>";
        } else {
            $message = "<h1>Delete Failed</h1><p>Error deleting file '$filename'.</p>";
        }
    } else {
        $message = "<h1>Delete Failed</h1><p>File '$filename' not found.</p>";
    }
}

// Handle file download
if (isset($_GET['action']) && $_GET['action'] === 'download' && isset($_GET['file'])) {
    $filename = $_GET['file'];
    $filepath = $upload_dir . $filename;

    if (file_exists($filepath)) {
        echo "Content-Type: application/octet-stream\r\n";
        echo "Content-Disposition: attachment; filename=\"" . $filename . "\"\r\n";
        echo "Content-Length: " . filesize($filepath) . "\r\n";
        echo "\r\n";
        readfile($filepath);
        exit;
    } else {
        $message = "<h1>Download Failed</h1><p>File '$filename' not found.</p>";
    }
}

// Get list of uploaded files
$files = [];
if (is_dir($upload_dir)) {
    $files = scandir($upload_dir);
    $files = array_diff($files, array('.', '..'));
}

// Output CGI headers
echo "Content-Type: text/html\r\n";
echo "\r\n";
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>File Upload</title>
</head>
<body>
    <?php if ($message): ?>
        <div><?php echo $message; ?></div>
    <?php endif; ?>

    <h1>File Upload</h1>
    <form method="post" enctype="multipart/form-data">
        <label for="file">Choose a file:</label>
        <input type="file" id="file" name="file" required>
        <br><br>
        <button type="submit">Upload</button>
    </form>

    <?php if (!empty($files)): ?>
    <h2>Uploaded Files</h2>
    <ul>
        <?php foreach ($files as $file): ?>
        <li>
            <a href="?action=download&file=<?php echo urlencode($file); ?>"><?php echo htmlspecialchars($file); ?></a>
            <a href="?action=delete&file=<?php echo urlencode($file); ?>" onclick="return confirm('Are you sure you want to delete <?php echo htmlspecialchars($file); ?>?')" style="color: red; margin-left: 10px;">×</a>
        </li>
        <?php endforeach; ?>
    </ul>
    <?php endif; ?>

    <p><a href="index.php">Back to Home</a></p>
</body>
</html>
