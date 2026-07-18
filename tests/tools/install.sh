#!/bin/bash
# HL7DB Installation Script
# Installs HL7DB server and tools to system directories

set -e  # Exit on error

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="${INSTALL_PREFIX}/bin"
CONF_DIR="${CONF_DIR:-/etc/hl7db}"
DATA_DIR="${DATA_DIR:-/var/lib/hl7db}"
LOG_DIR="${LOG_DIR:-/var/log/hl7db}"
SYSTEMD_DIR="/etc/systemd/system"
USER="hl7db"
GROUP="hl7db"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
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

log_info "HL7DB Installation Script"
log_info "========================="
echo ""

# Check if binaries exist
if [ ! -f "bin/hl7db" ]; then
    log_error "HL7DB server binary not found. Please run 'make' first."
    exit 1
fi

# Create system user if it doesn't exist
if ! id "$USER" &>/dev/null; then
    log_info "Creating system user: $USER"
    useradd -r -s /bin/false -d "$DATA_DIR" -c "HL7DB Server" "$USER"
else
    log_info "User '$USER' already exists"
fi

# Create directories
log_info "Creating directories..."
mkdir -p "$BIN_DIR"
mkdir -p "$CONF_DIR"
mkdir -p "$DATA_DIR"
mkdir -p "$DATA_DIR/channels"
mkdir -p "$LOG_DIR"

# Set ownership
chown -R "$USER:$GROUP" "$DATA_DIR"
chown -R "$USER:$GROUP" "$LOG_DIR"

# Install binaries
log_info "Installing binaries to $BIN_DIR..."
install -m 0755 bin/hl7db "$BIN_DIR/hl7db"

if [ -f "bin/web_queue_tester" ]; then
    install -m 0755 bin/web_queue_tester "$BIN_DIR/hl7db-web-tester"
fi

if [ -f "bin/test_queue_tester" ]; then
    install -m 0755 bin/test_queue_tester "$BIN_DIR/hl7db-tester"
fi

# Install configuration file
if [ ! -f "$CONF_DIR/hl7db.conf" ]; then
    log_info "Installing default configuration to $CONF_DIR/hl7db.conf..."
    if [ -f "hl7db.conf.sample" ]; then
        install -m 0644 hl7db.conf.sample "$CONF_DIR/hl7db.conf"
        # Update paths in config
        sed -i "s|data_directory = ./data/channels|data_directory = $DATA_DIR/channels|" "$CONF_DIR/hl7db.conf"
        sed -i "s|audit_log_path = ./data/audit.log|audit_log_path = $LOG_DIR/audit.log|" "$CONF_DIR/hl7db.conf"
        sed -i "s|log_file = ./data/hl7db.log|log_file = $LOG_DIR/hl7db.log|" "$CONF_DIR/hl7db.conf"
    else
        log_warn "Sample configuration file not found, skipping"
    fi
else
    log_info "Configuration file already exists at $CONF_DIR/hl7db.conf"
fi

# Install systemd service
if [ -d "$SYSTEMD_DIR" ]; then
    log_info "Installing systemd service..."
    cat > "$SYSTEMD_DIR/hl7db.service" <<EOF
[Unit]
Description=HL7DB Message Database Server
Documentation=https://github.com/yourusername/HL7DB
After=network.target

[Service]
Type=simple
User=$USER
Group=$GROUP
WorkingDirectory=$DATA_DIR
ExecStart=$BIN_DIR/hl7db --config $CONF_DIR/hl7db.conf
ExecReload=/bin/kill -HUP \$MAINPID
Restart=on-failure
RestartSec=5s

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$DATA_DIR $LOG_DIR

# Resource limits
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
EOF

    chmod 0644 "$SYSTEMD_DIR/hl7db.service"
    systemctl daemon-reload
    log_info "Systemd service installed"
else
    log_warn "Systemd not found, skipping service installation"
fi

# Create logrotate configuration
if [ -d "/etc/logrotate.d" ]; then
    log_info "Installing logrotate configuration..."
    cat > "/etc/logrotate.d/hl7db" <<EOF
$LOG_DIR/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 0640 $USER $GROUP
    sharedscripts
    postrotate
        systemctl reload hl7db > /dev/null 2>&1 || true
    endscript
}
EOF
fi

# Installation summary
echo ""
log_info "Installation complete!"
echo ""
echo "Installed files:"
echo "  Binary:        $BIN_DIR/hl7db"
echo "  Configuration: $CONF_DIR/hl7db.conf"
echo "  Data directory: $DATA_DIR"
echo "  Log directory:  $LOG_DIR"
echo ""
echo "Next steps:"
echo "  1. Review configuration: sudo nano $CONF_DIR/hl7db.conf"
echo "  2. Start the service:    sudo systemctl start hl7db"
echo "  3. Enable auto-start:    sudo systemctl enable hl7db"
echo "  4. Check status:         sudo systemctl status hl7db"
echo "  5. View logs:            sudo journalctl -u hl7db -f"
echo ""
log_info "To uninstall, run: sudo bash tools/uninstall.sh"
