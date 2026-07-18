#!/bin/bash
# HL7DB Uninstallation Script

set -e

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="${INSTALL_PREFIX}/bin"
CONF_DIR="${CONF_DIR:-/etc/hl7db}"
DATA_DIR="${DATA_DIR:-/var/lib/hl7db}"
LOG_DIR="${LOG_DIR:-/var/log/hl7db}"
SYSTEMD_DIR="/etc/systemd/system"
USER="hl7db"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    log_error "This script must be run as root (use sudo)"
    exit 1
fi

log_info "HL7DB Uninstallation Script"
log_info "============================"
echo ""

# Confirm uninstallation
read -p "This will remove HL7DB from your system. Continue? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    log_info "Uninstallation cancelled"
    exit 0
fi

# Stop and disable service
if [ -f "$SYSTEMD_DIR/hl7db.service" ]; then
    log_info "Stopping and disabling HL7DB service..."
    systemctl stop hl7db 2>/dev/null || true
    systemctl disable hl7db 2>/dev/null || true
    rm -f "$SYSTEMD_DIR/hl7db.service"
    systemctl daemon-reload
fi

# Remove binaries
log_info "Removing binaries..."
rm -f "$BIN_DIR/hl7db"
rm -f "$BIN_DIR/hl7db-web-tester"
rm -f "$BIN_DIR/hl7db-tester"
rm -f "$BIN_DIR/hl7db-cli"
rm -f "$BIN_DIR/hl7db-import"

# Remove logrotate config
rm -f "/etc/logrotate.d/hl7db"

# Optionally remove configuration
read -p "Remove configuration files in $CONF_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    log_info "Removing configuration..."
    rm -rf "$CONF_DIR"
fi

# Optionally remove data
read -p "Remove data directory $DATA_DIR? (WARNING: This deletes all messages!) (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    log_warn "Removing data directory..."
    rm -rf "$DATA_DIR"
fi

# Optionally remove logs
read -p "Remove log directory $LOG_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    log_info "Removing logs..."
    rm -rf "$LOG_DIR"
fi

# Optionally remove user
read -p "Remove system user '$USER'? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    log_info "Removing user $USER..."
    userdel "$USER" 2>/dev/null || true
fi

echo ""
log_info "Uninstallation complete!"
