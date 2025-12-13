#!/usr/bin/php
<?php
$upload_dir = __DIR__ . '/uploads/';
$message = '';

// Ensure upload directory exists
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0755, true);
}

// Parse QUERY_STRING manually for CGI mode (populates $_GET)
if (empty($_GET) && isset($_SERVER['QUERY_STRING']) && !empty($_SERVER['QUERY_STRING'])) {
    parse_str($_SERVER['QUERY_STRING'], $_GET);
}

// Parse multipart form data from stdin for CGI
function parse_multipart_stdin(&$debug) {
    $content_type = $_SERVER['CONTENT_TYPE'] ?? '';
    if (strpos($content_type, 'multipart/form-data') === false) {
        $debug .= "FAIL: Content-Type not multipart<br>";
        return null;
    }

    // Get boundary from Content-Type
    preg_match('/boundary=(.*)$/', $content_type, $matches);
    if (empty($matches[1])) {
        $debug .= "FAIL: No boundary in Content-Type<br>";
        return null;
    }
    $boundary = trim($matches[1]);
    $debug .= "Boundary: " . htmlspecialchars($boundary) . "<br>";

    // Read stdin using CONTENT_LENGTH to ensure we get all data
    $content_length = isset($_SERVER['CONTENT_LENGTH']) ? intval($_SERVER['CONTENT_LENGTH']) : 0;
    $debug .= "Expected content length: " . $content_length . " bytes<br>";

    if ($content_length > 0) {
        // Read exactly CONTENT_LENGTH bytes
        $raw_data = '';
        $remaining = $content_length;
        $stdin = fopen('php://stdin', 'rb');
        if ($stdin) {
            while ($remaining > 0 && !feof($stdin)) {
                $chunk_size = min(65536, $remaining);
                $chunk = fread($stdin, $chunk_size);
                if ($chunk === false) {
                    $debug .= "Read error at " . strlen($raw_data) . " bytes<br>";
                    break;
                }
                $raw_data .= $chunk;
                $remaining -= strlen($chunk);
            }
            fclose($stdin);
        }
    } else {
        // Fallback to file_get_contents
        $raw_data = file_get_contents('php://stdin');
    }
    $debug .= "STDIN size: " . strlen($raw_data) . " bytes<br>";

    if (empty($raw_data)) {
        $debug .= "FAIL: STDIN is empty<br>";
        return null;
    }

    // Show first 200 chars of stdin for debugging
    $debug .= "STDIN start: " . htmlspecialchars(substr($raw_data, 0, 200)) . "<br>";

    // Split by boundary - use the full boundary marker
    $boundary_marker = '--' . $boundary;
    $debug .= "Looking for marker: " . htmlspecialchars($boundary_marker) . "<br>";

    $parts = explode($boundary_marker, $raw_data);
    $debug .= "Parts found: " . count($parts) . "<br>";

    foreach ($parts as $idx => $part) {
        // Skip empty parts and closing boundary
        if (empty($part) || $part === "--\r\n" || $part === "--" || trim($part) === "--") continue;

        $debug .= "Part $idx length: " . strlen($part) . "<br>";

        // Check if this is the file part
        if (preg_match('/Content-Disposition:\s*form-data;\s*name="file";\s*filename="([^"]+)"/i', $part, $matches)) {
            $filename = $matches[1];
            $debug .= "Found file: " . htmlspecialchars($filename) . "<br>";

            // Get content after the double CRLF (headers end)
            $content_start = strpos($part, "\r\n\r\n");
            if ($content_start === false) {
                $debug .= "FAIL: No header/body separator found<br>";
                continue;
            }
            $content = substr($part, $content_start + 4);

            // Remove ONLY the trailing boundary marker (CRLF before next boundary)
            if (substr($content, -2) === "\r\n") {
                $content = substr($content, 0, -2);
            }

            $debug .= "Content size: " . strlen($content) . " bytes<br>";
            return ['filename' => $filename, 'content' => $content];
        }
    }
    $debug .= "FAIL: No file part found in any parts<br>";
    return null;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Debug info to display
    $debug_info = "Content-Type: " . ($_SERVER['CONTENT_TYPE'] ?? 'NOT SET') . "<br>";
    $debug_info .= "Content-Length: " . ($_SERVER['CONTENT_LENGTH'] ?? 'NOT SET') . "<br>";

    $file_data = parse_multipart_stdin($debug_info);

    if ($file_data) {
        $debug_info .= "Parsed filename: " . htmlspecialchars($file_data['filename']) . "<br>";
        $debug_info .= "Content size: " . strlen($file_data['content']) . " bytes<br>";
    } else {
        $debug_info .= "parse_multipart_stdin returned NULL<br>";
    }

    if ($file_data && !empty($file_data['filename'])) {
        $target = $upload_dir . basename($file_data['filename']);
        if (file_put_contents($target, $file_data['content']) !== false) {
            $message = "<h1>Upload Successful</h1>";
            $message .= "<p>File uploaded: " . htmlspecialchars(basename($file_data['filename'])) . "</p>";
            $message .= "<p><small>Debug: $debug_info</small></p>";
        } else {
            $message = "<h1>Upload Failed</h1><p>Error saving file.</p>";
            $message .= "<p><small>Debug: $debug_info</small></p>";
        }
    } else {
        $message = "<h1>Upload Failed</h1><p>No file received or invalid format.</p>";
        $message .= "<p><small>Debug: $debug_info</small></p>";
    }
}

// Handle file deletion
if ($_SERVER['REQUEST_METHOD'] === 'DELETE' || (isset($_GET['action']) && $_GET['action'] === 'delete' && isset($_GET['file']))) {
    $filename = $_GET['file'];
    $filepath = $upload_dir . basename($filename);  // Use basename for security

    if (file_exists($filepath)) {
        if (unlink($filepath)) {
            // Redirect to clean URL with success message
            echo "Status: 303 See Other\r\n";
            echo "Location: /file_upload.php?msg=deleted&name=" . urlencode(basename($filename)) . "\r\n";
            echo "\r\n";
            exit;
        } else {
            $message = "<h1>Delete Failed</h1><p>Error deleting file '" . htmlspecialchars($filename) . "'.</p>";
        }
    } else {
        $message = "<h1>Delete Failed</h1><p>File '" . htmlspecialchars($filename) . "' not found.</p>";
    }
}

// Show success message from redirect
if (isset($_GET['msg']) && $_GET['msg'] === 'deleted' && isset($_GET['name'])) {
    $message = "<h1>Delete Successful</h1><p>File '" . htmlspecialchars($_GET['name']) . "' has been deleted.</p>";
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
