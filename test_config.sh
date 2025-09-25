#!/bin/bash

# Configuration parsing test script for WebServ

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

WEBSERV_BIN="./webserv"

# Test valid configurations
test_valid_configs() {
    echo -e "${YELLOW}Testing valid configurations...${NC}"
    
    # Test 1: Basic valid config
    cat > /tmp/valid1.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    index index.html;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/valid1.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Basic valid configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: Basic valid configuration"
    fi
    
    # Test 2: Multiple servers
    cat > /tmp/valid2.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET POST;
    }
}

server {
    listen 8081;
    server_name example.com;
    root /tmp;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/valid2.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Multiple servers configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: Multiple servers configuration"
    fi
    
    # Test 3: CGI configuration
    cat > /tmp/valid3.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET POST;
    }
    
    location /cgi-bin/ {
        allowed_methods GET POST;
        cgi_path /usr/bin/python3;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/valid3.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: CGI configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: CGI configuration"
    fi
}

# Test invalid configurations
test_invalid_configs() {
    echo -e "${YELLOW}Testing invalid configurations...${NC}"
    
    # Test 1: Missing listen directive
    cat > /tmp/invalid1.conf << 'EOF'
server {
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if ! $WEBSERV_BIN /tmp/invalid1.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Missing listen directive rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Missing listen directive accepted"
    fi
    
    # Test 2: Invalid port number
    cat > /tmp/invalid2.conf << 'EOF'
server {
    listen 99999;
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if ! $WEBSERV_BIN /tmp/invalid2.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid port number rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Invalid port number accepted"
    fi
    
    # Test 3: Missing allowed_methods in location
    cat > /tmp/invalid3.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    
    location / {
        cgi_path /usr/bin/python3;
    }
}
EOF
    
    if ! $WEBSERV_BIN /tmp/invalid3.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Missing allowed_methods rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Missing allowed_methods accepted"
    fi
    
    # Test 4: Invalid HTTP method
    cat > /tmp/invalid4.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET POST PUT;
    }
}
EOF
    
    if ! $WEBSERV_BIN /tmp/invalid4.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid HTTP method rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Invalid HTTP method accepted"
    fi
    
    # Test 5: Malformed syntax
    cat > /tmp/invalid5.conf << 'EOF'
server {
    listen 8080;
    server_name localhost
    root /tmp;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if ! $WEBSERV_BIN /tmp/invalid5.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Malformed syntax rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Malformed syntax accepted"
    fi
}

# Test edge cases
test_edge_cases() {
    echo -e "${YELLOW}Testing edge cases...${NC}"
    
    # Test 1: Empty configuration file
    echo "" > /tmp/empty.conf
    
    if ! $WEBSERV_BIN /tmp/empty.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Empty configuration rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Empty configuration accepted"
    fi
    
    # Test 2: Non-existent configuration file
    if ! $WEBSERV_BIN /tmp/nonexistent.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Non-existent file rejected"
    else
        echo -e "${RED}✗ FAIL${NC}: Non-existent file accepted"
    fi
    
    # Test 3: Configuration with comments
    cat > /tmp/comments.conf << 'EOF'
# This is a comment
server {
    listen 8080; # Another comment
    server_name localhost;
    root /tmp;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/comments.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Configuration with comments accepted"
    else
        echo -e "${RED}✗ FAIL${NC}: Configuration with comments rejected"
    fi
}

# Test advanced features
test_advanced_features() {
    echo -e "${YELLOW}Testing advanced configuration features...${NC}"
    
    # Test 1: Error pages
    cat > /tmp/error_pages.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    error_page 404 /404.html;
    error_page 500 /500.html;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/error_pages.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Error pages configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: Error pages configuration"
    fi
    
    # Test 2: Autoindex
    cat > /tmp/autoindex.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    autoindex on;
    
    location / {
        allowed_methods GET;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/autoindex.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Autoindex configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: Autoindex configuration"
    fi
    
    # Test 3: Client max body size
    cat > /tmp/body_size.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    root /tmp;
    client_max_body_size 10M;
    
    location / {
        allowed_methods GET POST;
    }
}
EOF
    
    if $WEBSERV_BIN /tmp/body_size.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}: Client max body size configuration"
    else
        echo -e "${RED}✗ FAIL${NC}: Client max body size configuration"
    fi
}

# Cleanup function
cleanup() {
    rm -f /tmp/valid*.conf /tmp/invalid*.conf /tmp/empty.conf /tmp/comments.conf
    rm -f /tmp/error_pages.conf /tmp/autoindex.conf /tmp/body_size.conf
}

# Main execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}     Configuration Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    test_valid_configs
    test_invalid_configs
    test_edge_cases
    test_advanced_features
    
    cleanup
    
    echo -e "${GREEN}Configuration tests completed!${NC}"
}

main "$@"