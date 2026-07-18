#!/usr/bin/env python3
"""
Test HL7DB Persistence Reload
==============================
Tests that messages are reloaded from disk after server restart.
"""

import socket
import struct
import sys


def send_query(host, port, query):
    """Send a query to HL7DB and get the response."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((host, port))

        # Send request: 4-byte length + query string
        query_bytes = query.encode('utf-8')
        length = struct.pack('!I', len(query_bytes))
        sock.sendall(length + query_bytes)

        # Read response: 1-byte status + 4-byte length + data
        status_byte = sock.recv(1)
        if not status_byte:
            return None, "Connection closed"

        status = struct.unpack('B', status_byte)[0]

        length_bytes = sock.recv(4)
        if not length_bytes or len(length_bytes) < 4:
            return None, "Failed to read length"

        length = struct.unpack('!I', length_bytes)[0]

        # Read data
        data = b''
        while len(data) < length:
            chunk = sock.recv(length - len(data))
            if not chunk:
                break
            data += chunk

        response = data.decode('utf-8', errors='replace')
        return status, response

    finally:
        sock.close()


def main():
    HOST = 'localhost'
    PORT = 7777

    print("=" * 60)
    print("HL7DB Persistence Reload Test")
    print("=" * 60)

    # Test 1: Create the channel again (should reload from disk)
    print("\n[1] Creating channel 'test_channel' (should reload from disk)...")
    status, response = send_query(HOST, PORT, "CREATE CHANNEL test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success: {response}")

    # Test 2: Query messages - should have the 2 messages we inserted before restart
    print("\n[2] Querying messages (should find 2 persisted messages)...")
    status, response = send_query(HOST, PORT, "SELECT MSG_ID, PID.5 FROM test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1

    print(f"✓ Success:")
    print(response)

    # Check if we got 2 rows
    if "2 row(s)" in response:
        print("\n✓ Both messages were successfully reloaded from disk!")
    else:
        print("\n✗ Expected 2 messages, but got different count")
        return 1

    # Test 3: Verify the content
    print("\n[3] Verifying message content...")
    if "DOE^JOHN^M" in response and "SMITH^JANE^A" in response:
        print("✓ Both patient names found!")
    else:
        print("✗ Message content doesn't match expected")
        return 1

    # Test 4: POP a message
    print("\n[4] Popping one message...")
    status, response = send_query(HOST, PORT, "POP MESSAGE FROM test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success: Popped message")

    # Test 5: Verify only 1 message remains
    print("\n[5] Verifying 1 message remains...")
    status, response = send_query(HOST, PORT, "SELECT MSG_ID FROM test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1

    if "1 row(s)" in response:
        print("✓ Exactly 1 message remains as expected!")
    else:
        print(f"✗ Expected 1 message, got: {response}")
        return 1

    print("\n" + "=" * 60)
    print("✓ All persistence reload tests passed!")
    print("=" * 60)
    print("\nPersistence is working correctly:")
    print("  ✓ Messages are written to disk on INSERT")
    print("  ✓ Messages are reloaded from disk on channel creation")
    print("  ✓ In-memory operations work correctly after reload")

    return 0


if __name__ == '__main__':
    sys.exit(main())
