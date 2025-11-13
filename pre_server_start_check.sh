#!/bin/bash

echo "=== Pre-Server Start Check ==="
echo "Checking directory permissions and CGI socket status..."
echo

# Pre-execution checks
UPLOAD_DIR="/home/sean/projects/showcas/www/html/upload"
WEB_USER="www-data"

echo "0. Directory Permission Check:"

# Check if directory exists
if [ ! -d "$UPLOAD_DIR" ]; then
  echo "   ✗ Upload directory $UPLOAD_DIR does not exist."
  echo "   Run: mkdir -p $UPLOAD_DIR"
  exit 1
fi

# Check if the web user can write to the directory
sudo -u $WEB_USER test -w "$UPLOAD_DIR"
if [ $? -ne 0 ]; then
  echo "   ✗ $WEB_USER does not have write permission to $UPLOAD_DIR."
  echo "   Run: sudo chown -R $WEB_USER:$WEB_USER $UPLOAD_DIR"
  exit 2
fi

echo "   ✓ $WEB_USER can write to $UPLOAD_DIR"
echo "   Note: To return ownership to your user later, run: sudo chown -R \$USER:\$USER $UPLOAD_DIR"
echo

# Check PHP-FPM process
echo "1. PHP-FPM Status:"
if pgrep -f "php-fpm" >/dev/null; then
    echo "   ✓ PHP-FPM is running"
    php_fpm_pid=$(pgrep -f "php-fpm")
    echo "   ✓ Main PHP-FPM PID: $php_fpm_pid"
    
    # Check PHP-FPM socket
    echo "   PHP-FPM Socket:"
    if [ -S "/run/php/php8.3-fpm.sock" ]; then
        socket_perms=$(stat -c "%a %U:%G" /run/php/php8.3-fpm.sock)
        perms_num=$(stat -c "%a" /run/php/php8.3-fpm.sock)
        perms_readable=$(stat -c "%A" /run/php/php8.3-fpm.sock)
        owner=$(stat -c "%U" /run/php/php8.3-fpm.sock)
        group=$(stat -c "%G" /run/php/php8.3-fpm.sock)
        
        echo "   ✓ Socket exists: $perms_readable ($perms_num) owned by $owner:$group"
        
        # Check socket permissions - srw-rw---- means owner and group can read/write
        full_perms=$(stat -c "%a" /run/php/php8.3-fpm.sock)
        if [ "$full_perms" = "660" ]; then
            echo "   ✓ Socket permissions: 660 (owner/group can access)"
        else
            echo "   ⚠ Socket permissions: $full_perms (may cause access issues)"
        fi
    else
        echo "   ✗ PHP-FPM socket not found at /run/php/php8.3-fpm.sock"
    fi
else
    echo "   ✗ PHP-FPM is not running"
fi

echo

# Check fcgiwrap process (for Python CGI)
echo "2. fcgiwrap Status:"
if pgrep -f "fcgiwrap" >/dev/null; then
    echo "   ✓ fcgiwrap is running"
    fcgi_pid=$(pgrep -f "fcgiwrap")
    echo "   ✓ fcgiwrap PID: $fcgi_pid"
    
        # Check fcgiwrap socket
    echo "   fcgiwrap Socket:"
    if [ -S "/run/fcgiwrap.socket" ]; then
        socket_perms=$(stat -c "%a %U:%G" /run/fcgiwrap.socket)
        perms_num=$(stat -c "%a" /run/fcgiwrap.socket)
        perms_readable=$(stat -c "%A" /run/fcgiwrap.socket)
        owner=$(stat -c "%U" /run/fcgiwrap.socket)
        group=$(stat -c "%G" /run/fcgiwrap.socket)

        echo "   ✓ Socket exists: $perms_readable ($perms_num) owned by $owner:$group"
        echo "   - Note: Check socket permissions manually if Python CGI fails"
    else
        echo "   ✗ fcgiwrap socket not found at /run/fcgiwrap.socket"
    fi
else
    echo "   ✗ fcgiwrap is not running (Python CGI scripts won't work)"
fi

echo

# Check Nginx status
echo "3. Nginx Status:"
if pgrep nginx >/dev/null; then
    nginx_pids=$(pgrep nginx)
    echo "   ✓ Nginx is running (PIDs: $(echo $nginx_pids | tr '\n' ' '))"
    
    # Check nginx worker user
    nginx_workers=$(ps aux | grep "nginx: worker" | head -3)
    if echo "$nginx_workers" | grep -q www; then
        echo "   ✓ Nginx workers are running as www-data (correct permissions)"
    else
        echo "   ⚠ Nginx workers may not be running as www-data"
    fi
else
    echo "   - Nginx is not running"
fi

echo
echo "4. Commands to start services:"
echo "   # Start PHP-FPM (if needed):"
echo "   sudo /usr/sbin/php-fpm8.3"
echo
echo "   # Start fcgiwrap for Python CGI (if needed):"
echo "   sudo rm -f /run/fcgiwrap.socket && sudo fcgiwrap -s unix:/run/fcgiwrap.socket &"
echo
echo "   # Start Nginx with proper permissions:"
echo "   sudo nginx -p /home/sean/projects/showcas -c nginx.conf"
echo
echo "   # Check if server is working:"
echo "   curl http://localhost:3000"
