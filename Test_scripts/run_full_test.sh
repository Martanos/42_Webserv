#!/usr/bin/env bash
# Full integration test suite for 42 webserv, including ubuntu_tester.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)

WEBSERV_BIN="${PROJECT_ROOT}/webserv"
UBUNTU_TESTER="${PROJECT_ROOT}/Project_resources/ubuntu_tester"

TEST_PORT=${WEBSERV_TEST_PORT:-8080}
TEST_HOST="127.0.0.1"
CURL_MAX_TIME=${CURL_MAX_TIME:-10}
CURL_CONNECT_TIMEOUT=${CURL_CONNECT_TIMEOUT:-3}
CURL_RETRIES=${CURL_RETRIES:-0}
SERVER_START_TIMEOUT=${SERVER_START_TIMEOUT:-15}
SERVER_HEALTH_TIMEOUT=${SERVER_HEALTH_TIMEOUT:-3}
SERVER_MAX_RESTARTS=${SERVER_MAX_RESTARTS:-3}
RESTART_ON_FAILURE=${RESTART_ON_FAILURE:-1}

HEALTHCHECK_URL=${HEALTHCHECK_URL:-"http://${TEST_HOST}:${TEST_PORT}/"}

SERVER_PID=""
SERVER_LOG=""
CONFIG_FILE=""
CGI_RUNNER=""
CGI_SCRIPT=""
UPLOAD_TARGET="${PROJECT_ROOT}/www/put_test/test_script_upload.txt"
LAST_SERVER_LOG=""
SERVER_RESTARTS=0

TOTAL_TESTS=0
FAILED_TESTS=0
FAILED_DESCRIPTIONS=()

log() {
    printf '%s
' "$*"
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        log "[ERROR] Required command '$1' not found."
        exit 1
    fi
}

cleanup() {
	stop_server
	[[ -n "${CONFIG_FILE}" && -f "${CONFIG_FILE}" ]] && rm -f "${CONFIG_FILE}"
	[[ -n "${CGI_RUNNER}" && -f "${CGI_RUNNER}" ]] && rm -f "${CGI_RUNNER}"
	[[ -n "${CGI_SCRIPT}" && -f "${CGI_SCRIPT}" ]] && rm -f "${CGI_SCRIPT}"
	[[ -f "${UPLOAD_TARGET}" ]] && rm -f "${UPLOAD_TARGET}"
	if [[ -n "${SERVER_LOG}" && -f "${SERVER_LOG}" ]]; then
		LAST_SERVER_LOG="${SERVER_LOG}"
	fi
	if [[ -n "${LAST_SERVER_LOG}" && -f "${LAST_SERVER_LOG}" ]]; then
		mkdir -p "${PROJECT_ROOT}/Test_scripts/logs"
		cp "${LAST_SERVER_LOG}" "${PROJECT_ROOT}/Test_scripts/logs/last_server.log" >/dev/null 2>&1 || true
	fi
}

trap cleanup EXIT INT TERM

run_test() {
    local description=$1
    local callback=$2
	((++TOTAL_TESTS))
	if ! ensure_server_running; then
		log "[FAIL] ${description} (server unavailable before test)"
		dump_server_log
		FAILED_DESCRIPTIONS+=("${description} - server unavailable before test")
		((++FAILED_TESTS))
		return 0
	fi

	if "${callback}"; then
		if check_server_health; then
			log "[PASS] ${description}"
		else
			log "[FAIL] ${description} (health check failed after test)"
			dump_server_log
			FAILED_DESCRIPTIONS+=("${description} - post-test health failure")
			((++FAILED_TESTS))
			if [[ "${RESTART_ON_FAILURE}" == "1" ]]; then
				restart_server || true
			fi
		fi
	else
		log "[FAIL] ${description}"
		dump_server_log
		FAILED_DESCRIPTIONS+=("${description}")
		((++FAILED_TESTS))
		if [[ "${RESTART_ON_FAILURE}" == "1" ]]; then
			restart_server || true
		fi
	fi
	return 0
}

dump_server_log() {
	if [[ -n "${SERVER_LOG}" && -f "${SERVER_LOG}" ]]; then
		log "[INFO] Last 40 lines of server log (${SERVER_LOG}):"
		tail -n 40 "${SERVER_LOG}" || true
	fi
}

check_server_health() {
	curl --connect-timeout "${SERVER_HEALTH_TIMEOUT}" --max-time "${SERVER_HEALTH_TIMEOUT}" \
		-s -o /dev/null -I "${HEALTHCHECK_URL}"
}

restart_server() {
	((++SERVER_RESTARTS))
	if ((SERVER_RESTARTS > SERVER_MAX_RESTARTS)); then
		log "[ERROR] Maximum server restart attempts exceeded (${SERVER_MAX_RESTARTS})"
		return 1
	fi
	log "[WARN] Restarting server (attempt ${SERVER_RESTARTS}/${SERVER_MAX_RESTARTS})"
	if [[ -n "${SERVER_LOG}" && -f "${SERVER_LOG}" ]]; then
		mkdir -p "${PROJECT_ROOT}/Test_scripts/logs"
		cp "${SERVER_LOG}" "${PROJECT_ROOT}/Test_scripts/logs/server_restart_${SERVER_RESTARTS}.log" >/dev/null 2>&1 || true
	fi
	stop_server
	start_server
	return 0
}

ensure_server_running() {
	if [[ -z "${SERVER_PID}" ]] || ! kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
		log "[WARN] Server process not running."
		restart_server || return 1
	fi
	if ! nc -z "${TEST_HOST}" "${TEST_PORT}" >/dev/null 2>&1; then
		log "[WARN] Server port ${TEST_PORT} not responding."
		restart_server || return 1
	fi
	if ! check_server_health; then
		log "[WARN] Server health check failed before test."
		restart_server || return 1
	fi
	return 0
}

wait_for_port() {
    local host=$1
    local port=$2
	local timeout=${SERVER_START_TIMEOUT}
	local end=$((SECONDS + timeout))
	while ((SECONDS < end)); do
		if nc -z "${host}" "${port}" >/dev/null 2>&1; then
			return 0
		fi
		sleep 0.25
	done
	return 1
}

build_server() {
    log "[INFO] Building webserv"
    make -C "${PROJECT_ROOT}" >/dev/null
}

prepare_environment() {
    require_command make
    require_command curl
    require_command nc
    require_command timeout
    require_command yes

    [[ -d "${PROJECT_ROOT}/www/put_test" ]] || mkdir -p "${PROJECT_ROOT}/www/put_test"
    [[ -d "${PROJECT_ROOT}/www/virtual_host" ]] || mkdir -p "${PROJECT_ROOT}/www/virtual_host"
    [[ -d "${PROJECT_ROOT}/www/cgi-bin" ]] || mkdir -p "${PROJECT_ROOT}/www/cgi-bin"
	[[ -d "${PROJECT_ROOT}/Test_scripts/logs" ]] || mkdir -p "${PROJECT_ROOT}/Test_scripts/logs"

    echo "<html><body><h1>Virtual Host Works</h1></body></html>" > "${PROJECT_ROOT}/www/virtual_host/index.html"

    CGI_SCRIPT=$(mktemp "${PROJECT_ROOT}/www/cgi-bin/indexXXXX.cgi")
    cat <<'CGISCRIPT' > "${CGI_SCRIPT}"
#!/usr/bin/env python3
import os
print("Status: 200 OK")
print("Content-Type: text/plain
")
print("Hello from CGI")
for key in sorted(os.environ):
    if key.startswith("HTTP_"):
        print(f"{key}={os.environ[key]}")
CGISCRIPT
    chmod +x "${CGI_SCRIPT}"

    CGI_RUNNER=$(mktemp "${PROJECT_ROOT}/Test_scripts/cgi_runnerXXXX")
    cat <<'CGIRUNNER' > "${CGI_RUNNER}"
#!/bin/sh
exec "$@"
CGIRUNNER
    chmod +x "${CGI_RUNNER}"
}

generate_config() {
    CONFIG_FILE=$(mktemp "${PROJECT_ROOT}/Test_scripts/full_suite.XXXX.conf")
    cat <<EOF > "${CONFIG_FILE}"
server {
    listen ${TEST_PORT};
    server_name localhost;
    root ${PROJECT_ROOT}/www;
    index index.html;
    autoindex off;

    location / {
        allowed_methods GET;
    }

    location /directory/ {
        allowed_methods GET;
        root ${PROJECT_ROOT}/www/YoupiBanane;
        index youpi.bad_extension;
        autoindex on;
    }

    location /upload/ {
        allowed_methods GET PUT DELETE;
        root ${PROJECT_ROOT}/www/put_test;
    }

    location /cgi-bin/ {
        allowed_methods GET POST;
        root ${PROJECT_ROOT}/www/cgi-bin;
        cgi_path ${CGI_RUNNER};
    }

    location /post_body {
        allowed_methods POST;
        client_max_body_size 100;
    }
}

server {
    listen ${TEST_PORT};
    server_name virtual.test;
    root ${PROJECT_ROOT}/www/virtual_host;
    index index.html;
    location / {
        allowed_methods GET;
    }
}
EOF
}

start_server() {
	SERVER_LOG=$(mktemp "${PROJECT_ROOT}/Test_scripts/logs/webserv.XXXX.log")
    "${WEBSERV_BIN}" "${CONFIG_FILE}" > "${SERVER_LOG}" 2>&1 &
    SERVER_PID=$!
    if ! wait_for_port "${TEST_HOST}" "${TEST_PORT}"; then
        log "[ERROR] Server failed to start. See log: ${SERVER_LOG}"
        cat "${SERVER_LOG}" >&2 || true
        exit 1
    fi
	LAST_SERVER_LOG="${SERVER_LOG}"
}

stop_server() {
    if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
        kill "${SERVER_PID}" >/dev/null 2>&1 || true
        wait "${SERVER_PID}" >/dev/null 2>&1 || true
    fi
    SERVER_PID=""
}

curl_request() {
    local method=$1 url=$2 expected_code=$3 expected_substring=$4
    shift 4
    local output_file
    output_file=$(mktemp)
    local code
	code=$(curl --connect-timeout "${CURL_CONNECT_TIMEOUT}" --max-time "${CURL_MAX_TIME}" \
		--retry "${CURL_RETRIES}" -sS -o "${output_file}" -w "%{http_code}" -X "${method}" "$@" "${url}") || true
    local success=0
    if [[ "${code}" == "${expected_code}" ]]; then
        if [[ -z "${expected_substring}" ]] || grep -q "${expected_substring}" "${output_file}"; then
            success=1
        fi
    fi
    rm -f "${output_file}"
    [[ ${success} -eq 1 ]]
}

test_get_root() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/" 200 "Index Page"
}

test_virtual_host() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/" 200 "Virtual Host Works" -H "Host: virtual.test"
}

test_autoindex() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/directory/" 200 "Index of"
}

test_error_page() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/does-not-exist" 404 "404"
}

test_method_not_allowed() {
    curl_request DELETE "http://${TEST_HOST}:${TEST_PORT}/" 405 "Method Not Allowed"
}

test_put_create() {
    local code output_file
    output_file=$(mktemp)
	code=$(curl --connect-timeout "${CURL_CONNECT_TIMEOUT}" --max-time "${CURL_MAX_TIME}" \
		--retry "${CURL_RETRIES}" -sS -o "${output_file}" -w "%{http_code}" \
		-X PUT --data 'Hello from PUT' "http://${TEST_HOST}:${TEST_PORT}/upload/test_script_upload.txt") || true
    local ok=0
    if [[ "${code}" == "201" && -f "${UPLOAD_TARGET}" ]] && grep -q "Hello from PUT" "${UPLOAD_TARGET}"; then
        ok=1
    fi
    rm -f "${output_file}"
    [[ ${ok} -eq 1 ]]
}

test_get_uploaded() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/upload/test_script_upload.txt" 200 "Hello from PUT"
}

test_put_overwrite() {
    local code output_file
    output_file=$(mktemp)
	code=$(curl --connect-timeout "${CURL_CONNECT_TIMEOUT}" --max-time "${CURL_MAX_TIME}" \
		--retry "${CURL_RETRIES}" -sS -o "${output_file}" -w "%{http_code}" \
		-X PUT --data 'Updated content' "http://${TEST_HOST}:${TEST_PORT}/upload/test_script_upload.txt") || true
    local ok=0
    if [[ "${code}" == "204" && -f "${UPLOAD_TARGET}" ]] && grep -q "Updated content" "${UPLOAD_TARGET}"; then
        ok=1
    fi
    rm -f "${output_file}"
    [[ ${ok} -eq 1 ]]
}

test_delete_uploaded() {
    local output_file code
    output_file=$(mktemp)
	code=$(curl --connect-timeout "${CURL_CONNECT_TIMEOUT}" --max-time "${CURL_MAX_TIME}" \
		--retry "${CURL_RETRIES}" -sS -o "${output_file}" -w "%{http_code}" \
		-X DELETE "http://${TEST_HOST}:${TEST_PORT}/upload/test_script_upload.txt") || true
    local ok=0
    if [[ "${code}" == "200" && ! -f "${UPLOAD_TARGET}" ]]; then
        ok=1
    fi
    rm -f "${output_file}"
    [[ ${ok} -eq 1 ]]
}

test_cgi_get() {
    curl_request GET "http://${TEST_HOST}:${TEST_PORT}/cgi-bin/$(basename "${CGI_SCRIPT}")" 200 "Hello from CGI"
}

run_ubuntu_tester() {
    if [[ ! -x "${UBUNTU_TESTER}" ]]; then
        log "[WARN] ubuntu_tester not found or not executable, skipping."
        return 0
    fi
    local timeout_value=${UBUNTU_TESTER_TIMEOUT:-120}
    if [[ "${SKIP_UBUNTU_TESTER:-0}" == "1" ]]; then
        log "[INFO] Skipping ubuntu_tester as requested."
        return 0
    fi
    log "[INFO] Running ubuntu_tester (timeout ${timeout_value}s)"
    if timeout "${timeout_value}" bash -lc "cd '${PROJECT_ROOT}' && yes '' | '${UBUNTU_TESTER}' http://${TEST_HOST}:${TEST_PORT}"; then
        return 0
    else
        log "[ERROR] ubuntu_tester failed (see output above)."
        return 1
    fi
}

main() {
    prepare_environment
    build_server
    generate_config
    SERVER_RESTARTS=0
    LAST_SERVER_LOG=""
    start_server

    run_test "GET / serves index" test_get_root
    run_test "Virtual host selection" test_virtual_host
    run_test "Autoindex listing" test_autoindex
    run_test "Custom 404 error page" test_error_page
    run_test "Method not allowed handling" test_method_not_allowed
    run_test "PUT creates new resource" test_put_create
    run_test "GET retrieves uploaded file" test_get_uploaded
    run_test "PUT overwrites existing resource" test_put_overwrite
    run_test "DELETE removes uploaded file" test_delete_uploaded
    run_test "CGI execution" test_cgi_get
    run_test "ubuntu_tester regression suite" run_ubuntu_tester

    stop_server

    log ""
    log "===================="
    log "Test summary: ${TOTAL_TESTS} run, ${FAILED_TESTS} failed"
    if [[ ${FAILED_TESTS} -gt 0 ]]; then
        log "Failed tests:"
        for desc in "${FAILED_DESCRIPTIONS[@]}"; do
            log "  - ${desc}"
        done
        log "Server log preserved at: ${PROJECT_ROOT}/Test_scripts/logs/last_server.log"
        exit 1
    fi
    log "All tests passed!"
}

main "$@"
