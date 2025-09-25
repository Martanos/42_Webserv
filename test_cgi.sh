#!/bin/bash

# CGI-specific test script for WebServ

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

WEBSERV_BIN="./webserv"
TEST_CONFIG="test.conf"
TEST_PORT=8081
TEST_DIR="/tmp/webserv_cgi_test"

# Create CGI test environment
setup_cgi_test() {
    echo -e "${BLUE}Setting up CGI test environment...${NC}"
    mkdir -p $TEST_DIR/cgi-bin
    
    # Create various CGI test scripts
    
    # 1. Basic CGI script
    cat > $TEST_DIR/cgi-bin/basic.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain")
print()
print("Hello from CGI!")
EOF
    
    # 2. Environment variables CGI script
    cat > $TEST_DIR/cgi-bin/env.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI Environment Variables</h1>")
for key, value in sorted(os.environ.items()):
    if key.startswith(('HTTP_', 'REQUEST_', 'SERVER_', 'SCRIPT_')):
        print(f"<p><strong>{key}</strong>: {value}</p>")
print("</body></html>")
EOF
    
    # 3. Query string CGI script
    cat > $TEST_DIR/cgi-bin/query.py << 'EOF'
#!/usr/bin/env python3
import os
import urllib.parse
print("Content-Type: text/html")
print()
query_string = os.environ.get('QUERY_STRING', '')
params = urllib.parse.parse_qs(query_string)
print("<html><body>")
print("<h1>Query String Parameters</h1>")
if params:
    for key, values in params.items():
        print(f"<p><strong>{key}</strong>: {', '.join(values)}</p>")
else:
    print("<p>No query parameters</p>")
print("</body></html>")
EOF
    
    # 4. POST data CGI script
    cat > $TEST_DIR/cgi-bin/post.py << 'EOF'
#!/usr/bin/env python3
import os
import sys
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>POST Data</h1>")
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
if content_length > 0:
    post_data = sys.stdin.read(content_length)
    print(f"<p>POST data: {post_data}</p>")
else:
    print("<p>No POST data</p>")
print("</body></html>")
EOF
    
    # 5. Error CGI script (returns 500)
    cat > $TEST_DIR/cgi-bin/error.py << 'EOF'
#!/usr/bin/env python3
import sys
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI Error Test</h1>")
print("<p>This script intentionally causes an error</p>")
# Cause an error
sys.exit(1)
EOF
    
    # Make all scripts executable
    chmod +x $TEST_DIR/cgi-bin/*.py
    
    # Update config to use test directory
    sed "s|/home/malee/42_Projects/webserv/www|$TEST_DIR|g" $TEST_CONFIG > /tmp/cgi_test_config.conf
}

# Test CGI functionality
test_cgi_basic() {
    echo -e "${YELLOW}Testing basic CGI functionality...${NC}"
    
    local response=$(curl -s http://localhost:$TEST_PORT/cgi-bin/basic.py)
    if echo "$response" | grep -q "Hello from CGI!"; then
        echo -e "${GREEN}✓ PASS${NC}: Basic CGI execution"
    else
        echo -e "${RED}✗ FAIL${NC}: Basic CGI execution"
    fi
}

test_cgi_environment() {
    echo -e "${YELLOW}Testing CGI environment variables...${NC}"
    
    local response=$(curl -s http://localhost:$TEST_PORT/cgi-bin/env.py)
    if echo "$response" | grep -q "REQUEST_METHOD"; then
        echo -e "${GREEN}✓ PASS${NC}: CGI environment variables"
    else
        echo -e "${RED}✗ FAIL${NC}: CGI environment variables"
    fi
}

test_cgi_query_string() {
    echo -e "${YELLOW}Testing CGI query string handling...${NC}"
    
    local response=$(curl -s "http://localhost:$TEST_PORT/cgi-bin/query.py?name=test&value=123")
    if echo "$response" | grep -q "name.*test"; then
        echo -e "${GREEN}✓ PASS${NC}: CGI query string handling"
    else
        echo -e "${RED}✗ FAIL${NC}: CGI query string handling"
    fi
}

test_cgi_post_data() {
    echo -e "${YELLOW}Testing CGI POST data handling...${NC}"
    
    local response=$(curl -s -X POST -d "test=data&more=info" http://localhost:$TEST_PORT/cgi-bin/post.py)
    if echo "$response" | grep -q "test=data"; then
        echo -e "${GREEN}✓ PASS${NC}: CGI POST data handling"
    else
        echo -e "${RED}✗ FAIL${NC}: CGI POST data handling"
    fi
}

test_cgi_error_handling() {
    echo -e "${YELLOW}Testing CGI error handling...${NC}"
    
    local status_code=$(curl -s -w '%{http_code}' -o /dev/null http://localhost:$TEST_PORT/cgi-bin/error.py)
    if [ "$status_code" = "500" ]; then
        echo -e "${GREEN}✓ PASS${NC}: CGI error handling"
    else
        echo -e "${RED}✗ FAIL${NC}: CGI error handling (got status: $status_code)"
    fi
}

# Main execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}        CGI Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    setup_cgi_test
    
    # Start webserv
    echo -e "${BLUE}Starting WebServ...${NC}"
    $WEBSERV_BIN /tmp/cgi_test_config.conf > /tmp/webserv_cgi.log 2>&1 &
    WEBSERV_PID=$!
    
    # Wait for server to start
    sleep 3
    
    # Run tests
    test_cgi_basic
    test_cgi_environment
    test_cgi_query_string
    test_cgi_post_data
    test_cgi_error_handling
    
    # Cleanup
    echo -e "${BLUE}Cleaning up...${NC}"
    kill $WEBSERV_PID 2>/dev/null
    rm -rf $TEST_DIR
    rm -f /tmp/cgi_test_config.conf
    
    echo -e "${GREEN}CGI tests completed!${NC}"
}

main "$@"