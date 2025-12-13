#!/usr/bin/php
<?php
// Output CGI headers first
echo "Content-Type: text/plain\r\n";
echo "\r\n";

echo "=== CGI Environment Debug ===\n\n";

echo "REQUEST_METHOD: " . ($_SERVER['REQUEST_METHOD'] ?? 'NOT SET') . "\n";
echo "CONTENT_TYPE: " . ($_SERVER['CONTENT_TYPE'] ?? 'NOT SET') . "\n";
echo "CONTENT_LENGTH: " . ($_SERVER['CONTENT_LENGTH'] ?? 'NOT SET') . "\n";
echo "SCRIPT_NAME: " . ($_SERVER['SCRIPT_NAME'] ?? 'NOT SET') . "\n";
echo "PATH_INFO: " . ($_SERVER['PATH_INFO'] ?? 'NOT SET') . "\n";
echo "QUERY_STRING: " . ($_SERVER['QUERY_STRING'] ?? 'NOT SET') . "\n";

echo "\n=== All SERVER Variables ===\n";
foreach ($_SERVER as $key => $value) {
    echo "$key: $value\n";
}

echo "\n=== STDIN Content ===\n";
$stdin = file_get_contents('php://stdin');
echo "STDIN Length: " . strlen($stdin) . " bytes\n";

if (strlen($stdin) > 0) {
    echo "\nFirst 500 bytes of STDIN:\n";
    echo "---START---\n";
    echo substr($stdin, 0, 500);
    echo "\n---END---\n";

    if (strlen($stdin) > 500) {
        echo "\nLast 200 bytes of STDIN:\n";
        echo "---START---\n";
        echo substr($stdin, -200);
        echo "\n---END---\n";
    }
} else {
    echo "STDIN is EMPTY!\n";
}

echo "\n=== Analysis ===\n";
if (empty($_SERVER['CONTENT_TYPE'])) {
    echo "ERROR: CONTENT_TYPE not set!\n";
} elseif (strpos($_SERVER['CONTENT_TYPE'], 'multipart/form-data') !== false) {
    echo "Content-Type is multipart/form-data - good\n";

    // Check for boundary
    if (preg_match('/boundary=(.*)$/', $_SERVER['CONTENT_TYPE'], $matches)) {
        echo "Boundary found: " . $matches[1] . "\n";
    } else {
        echo "ERROR: No boundary found in Content-Type!\n";
    }
} else {
    echo "Content-Type is: " . $_SERVER['CONTENT_TYPE'] . "\n";
}

if (empty($_SERVER['CONTENT_LENGTH']) || $_SERVER['CONTENT_LENGTH'] == '0') {
    echo "WARNING: CONTENT_LENGTH is 0 or not set!\n";
}

if (strlen($stdin) == 0 && !empty($_SERVER['CONTENT_LENGTH']) && $_SERVER['CONTENT_LENGTH'] > 0) {
    echo "ERROR: CONTENT_LENGTH says " . $_SERVER['CONTENT_LENGTH'] . " but STDIN is empty!\n";
    echo "This suggests the server is not passing the request body to CGI.\n";
}
?>
