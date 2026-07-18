#!/bin/bash
#
# generate-certs.sh - Generate self-signed TLS certificates for HL7DB
#
# For production use, replace with properly signed certificates from a trusted CA
#

set -e

CERT_DIR="/etc/hl7db"
KEY_FILE="$CERT_DIR/server-key.pem"
CERT_FILE="$CERT_DIR/server-cert.pem"

echo "Generating TLS certificates for HL7DB..."
echo "========================================="
echo ""

# Create certificate directory
echo "Creating certificate directory: $CERT_DIR"
sudo mkdir -p "$CERT_DIR"
sudo chmod 755 "$CERT_DIR"

# Generate self-signed certificate
echo "Generating 4096-bit RSA self-signed certificate..."
sudo openssl req -x509 \
    -newkey rsa:4096 \
    -keyout "$KEY_FILE" \
    -out "$CERT_FILE" \
    -days 365 \
    -nodes \
    -subj "/C=US/ST=State/L=City/O=Healthcare/OU=IT/CN=hl7db.local"

# Set proper permissions
echo "Setting secure permissions..."
sudo chmod 600 "$KEY_FILE"
sudo chmod 644 "$CERT_FILE"

echo ""
echo "✓ Certificate generated successfully!"
echo ""
echo "Certificate: $CERT_FILE"
echo "Private Key: $KEY_FILE"
echo ""
echo "⚠️  WARNING: This is a self-signed certificate for testing only!"
echo "   For production use, obtain certificates from a trusted Certificate Authority."
echo ""
