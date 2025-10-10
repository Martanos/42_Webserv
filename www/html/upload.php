<?php
$upload_dir = 'upload/';
$message = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['file'])) {
    $file = $_FILES['file'];
    if ($file['error'] === UPLOAD_ERR_OK) {
        // Move to upload directory
        $target = $upload_dir . basename($file['name']);
        if (move_uploaded_file($file['tmp_name'], $target)) {
            $message = "<h1>Upload Successful</h1>";
            $message .= "<p>File uploaded: " . htmlspecialchars(basename($file['name'])) . "</p>";
        } else {
            $message = "<h1>Upload Failed</h1><p>Error moving file.</p>";
        }
    } else {
        $message = "<h1>Upload Failed</h1><p>Error: " . $file['error'] . "</p>";
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
        header('Content-Type: application/octet-stream');
        header('Content-Disposition: attachment; filename="' . $filename . '"');
        header('Content-Length: ' . filesize($filepath));
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
