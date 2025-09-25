#!/bin/bash

# Comprehensive test runner for WebServ project
# Runs all test suites and provides detailed reporting

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
TEST_SUITES=()

# Function to run a test suite and track results
run_test_suite() {
    local suite_name="$1"
    local suite_script="$2"
    local description="$3"
    
    echo -e "${PURPLE}========================================${NC}"
    echo -e "${PURPLE}Running: $suite_name${NC}"
    echo -e "${PURPLE}Description: $description${NC}"
    echo -e "${PURPLE}========================================${NC}"
    
    if [ -f "$suite_script" ]; then
        if bash "$suite_script"; then
            echo -e "${GREEN}✓ $suite_name completed successfully${NC}"
            TEST_SUITES+=("$suite_name: PASSED")
        else
            echo -e "${RED}✗ $suite_name failed${NC}"
            TEST_SUITES+=("$suite_name: FAILED")
        fi
    else
        echo -e "${RED}✗ Test script $suite_script not found${NC}"
        TEST_SUITES+=("$suite_name: SCRIPT_NOT_FOUND")
    fi
    
    echo ""
}

# Function to check prerequisites
check_prerequisites() {
    echo -e "${BLUE}Checking prerequisites...${NC}"
    
    local missing_tools=()
    
    # Check for required tools
    command -v curl >/dev/null 2>&1 || missing_tools+=("curl")
    command -v nc >/dev/null 2>&1 || missing_tools+=("netcat")
    command -v python3 >/dev/null 2>&1 || missing_tools+=("python3")
    command -v bc >/dev/null 2>&1 || missing_tools+=("bc")
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
        echo -e "${YELLOW}Please install the missing tools and try again.${NC}"
        exit 1
    fi
    
    # Check if webserv binary exists
    if [ ! -f "./webserv" ]; then
        echo -e "${RED}WebServ binary not found. Please run 'make' first.${NC}"
        exit 1
    fi
    
    # Check if test config exists
    if [ ! -f "test.conf" ]; then
        echo -e "${RED}Test configuration file 'test.conf' not found.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ All prerequisites met${NC}"
    echo ""
}

# Function to run basic functionality tests
run_basic_tests() {
    echo -e "${CYAN}Running basic functionality tests...${NC}"
    
    # Test 1: Server startup
    echo -e "${YELLOW}Testing server startup...${NC}"
    if timeout 10 ./webserv test.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Server starts successfully${NC}"
    else
        echo -e "${RED}✗ Server startup failed${NC}"
    fi
    
    # Test 2: Configuration parsing
    echo -e "${YELLOW}Testing configuration parsing...${NC}"
    if ./webserv test.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Configuration parsing works${NC}"
    else
        echo -e "${RED}✗ Configuration parsing failed${NC}"
    fi
    
    # Test 3: Invalid configuration handling
    echo -e "${YELLOW}Testing invalid configuration handling...${NC}"
    echo "invalid config" > /tmp/invalid.conf
    if ! ./webserv /tmp/invalid.conf > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Invalid configuration properly rejected${NC}"
    else
        echo -e "${RED}✗ Invalid configuration accepted${NC}"
    fi
    rm -f /tmp/invalid.conf
    
    echo ""
}

# Function to run HTTP method tests
run_http_method_tests() {
    echo -e "${CYAN}Running HTTP method tests...${NC}"
    
    # Start server
    ./webserv test.conf > /tmp/webserv_test.log 2>&1 &
    local server_pid=$!
    sleep 3
    
    # Test GET method
    if curl -s http://localhost:8080/ > /dev/null; then
        echo -e "${GREEN}✓ GET method works${NC}"
    else
        echo -e "${RED}✗ GET method failed${NC}"
    fi
    
    # Test POST method (should be 405 on root location)
    local post_status=$(curl -s -w '%{http_code}' -o /dev/null -X POST http://localhost:8080/)
    if [ "$post_status" = "405" ]; then
        echo -e "${GREEN}✓ POST method properly rejected (405)${NC}"
    else
        echo -e "${RED}✗ POST method handling incorrect (got $post_status)${NC}"
    fi
    
    # Test DELETE method (should be 405 on root location)
    local delete_status=$(curl -s -w '%{http_code}' -o /dev/null -X DELETE http://localhost:8080/)
    if [ "$delete_status" = "405" ]; then
        echo -e "${GREEN}✓ DELETE method properly rejected (405)${NC}"
    else
        echo -e "${RED}✗ DELETE method handling incorrect (got $delete_status)${NC}"
    fi
    
    # Test unsupported method
    local put_status=$(curl -s -w '%{http_code}' -o /dev/null -X PUT http://localhost:8080/)
    if [ "$put_status" = "405" ]; then
        echo -e "${GREEN}✓ Unsupported method properly rejected (405)${NC}"
    else
        echo -e "${RED}✗ Unsupported method handling incorrect (got $put_status)${NC}"
    fi
    
    # Stop server
    kill $server_pid 2>/dev/null
    echo ""
}

# Function to run error handling tests
run_error_handling_tests() {
    echo -e "${CYAN}Running error handling tests...${NC}"
    
    # Start server
    ./webserv test.conf > /tmp/webserv_test.log 2>&1 &
    local server_pid=$!
    sleep 3
    
    # Test 404 error
    local not_found_status=$(curl -s -w '%{http_code}' -o /dev/null http://localhost:8080/nonexistent.html)
    if [ "$not_found_status" = "404" ]; then
        echo -e "${GREEN}✓ 404 error handling works${NC}"
    else
        echo -e "${RED}✗ 404 error handling failed (got $not_found_status)${NC}"
    fi
    
    # Test malformed request
    echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080 > /tmp/response 2>/dev/null
    if grep -q "HTTP/1.1" /tmp/response; then
        echo -e "${GREEN}✓ Malformed request handling works${NC}"
    else
        echo -e "${RED}✗ Malformed request handling failed${NC}"
    fi
    
    # Stop server
    kill $server_pid 2>/dev/null
    rm -f /tmp/response
    echo ""
}

# Function to run file serving tests
run_file_serving_tests() {
    echo -e "${CYAN}Running file serving tests...${NC}"
    
    # Start server
    ./webserv test.conf > /tmp/webserv_test.log 2>&1 &
    local server_pid=$!
    sleep 3
    
    # Test serving existing file
    if curl -s http://localhost:8080/index.html | grep -q "html"; then
        echo -e "${GREEN}✓ Static file serving works${NC}"
    else
        echo -e "${RED}✗ Static file serving failed${NC}"
    fi
    
    # Test serving non-existent file
    local status=$(curl -s -w '%{http_code}' -o /dev/null http://localhost:8080/nonexistent.html)
    if [ "$status" = "404" ]; then
        echo -e "${GREEN}✓ Non-existent file properly handled (404)${NC}"
    else
        echo -e "${RED}✗ Non-existent file handling incorrect (got $status)${NC}"
    fi
    
    # Stop server
    kill $server_pid 2>/dev/null
    echo ""
}

# Function to generate test report
generate_report() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}           TEST REPORT${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    echo -e "${CYAN}Test Suite Results:${NC}"
    for suite in "${TEST_SUITES[@]}"; do
        if [[ $suite == *"PASSED"* ]]; then
            echo -e "${GREEN}✓ $suite${NC}"
        elif [[ $suite == *"FAILED"* ]]; then
            echo -e "${RED}✗ $suite${NC}"
        else
            echo -e "${YELLOW}⚠ $suite${NC}"
        fi
    done
    
    echo ""
    echo -e "${CYAN}Summary:${NC}"
    local total_suites=${#TEST_SUITES[@]}
    local passed_suites=0
    local failed_suites=0
    
    for suite in "${TEST_SUITES[@]}"; do
        if [[ $suite == *"PASSED"* ]]; then
            passed_suites=$((passed_suites + 1))
        elif [[ $suite == *"FAILED"* ]]; then
            failed_suites=$((failed_suites + 1))
        fi
    done
    
    echo -e "Total Test Suites: $total_suites"
    echo -e "${GREEN}Passed: $passed_suites${NC}"
    echo -e "${RED}Failed: $failed_suites${NC}"
    
    if [ $failed_suites -eq 0 ]; then
        echo -e "${GREEN}🎉 All test suites passed!${NC}"
        return 0
    else
        echo -e "${RED}❌ Some test suites failed.${NC}"
        return 1
    fi
}

# Function to cleanup
cleanup() {
    echo -e "${BLUE}Cleaning up...${NC}"
    pkill -f webserv 2>/dev/null
    rm -f /tmp/webserv_test.log
    rm -f /tmp/invalid.conf
    rm -f /tmp/response
}

# Set up trap for cleanup on exit
trap cleanup EXIT

# Main execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}      WebServ Test Runner${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    # Check prerequisites
    check_prerequisites
    
    # Run basic tests
    run_basic_tests
    
    # Run HTTP method tests
    run_http_method_tests
    
    # Run error handling tests
    run_error_handling_tests
    
    # Run file serving tests
    run_file_serving_tests
    
    # Run comprehensive test suites
    run_test_suite "Configuration Tests" "./test_config.sh" "Tests configuration file parsing and validation"
    run_test_suite "CGI Tests" "./test_cgi.sh" "Tests CGI functionality and environment variables"
    run_test_suite "Performance Tests" "./test_performance.sh" "Tests server performance and concurrent connections"
    run_test_suite "Comprehensive Tests" "./test_suite.sh" "Tests all major functionality"
    
    # Generate report
    generate_report
}

main "$@"