#!/usr/bin/env python3
"""
Test HL7DB Persistence
======================
Tests that messages persist across server restarts.
"""

import socket
import struct
import time
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
    print("HL7DB Persistence Test")
    print("=" * 60)

    # Test 1: Create channel
    print("\n[1] Creating channel 'test_channel'...")
    status, response = send_query(HOST, PORT, "CREATE CHANNEL test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success: {response}")

    # Test 2: Insert sample HL7 message
    print("\n[2] Inserting HL7 message...")
    hl7_msg = r"MSH|^~\\&|EPIC|EPICADT|SMS|SMSADT|20240115120000||ADT^A04|123456|P|2.5\rPID|1||123456789||DOE^JOHN^M||19800101|M|||123 MAIN ST^^ANYTOWN^CA^12345||555-1234|"
    query = f"INSERT MESSAGE INTO test_channel VALUES ('{hl7_msg}')"
    status, response = send_query(HOST, PORT, query)
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success: {response}")

    # Test 3: Verify message is in memory
    print("\n[3] Querying message from memory...")
    status, response = send_query(HOST, PORT, "SELECT PID.5 FROM test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success:")
    print(response)

    # Test 4: Check persistence file exists
    print("\n[4] Checking persistence file...")
    import os
    persist_file = "./data/channels/test_channel/messages.dat"
    if not os.path.exists(persist_file):
        print(f"✗ Persistence file not found: {persist_file}")
        return 1

    with open(persist_file, 'r') as f:
        content = f.read()
        print(f"✓ Persistence file exists with {len(content)} bytes")
        print(f"Content preview: {content[:100]}...")

    # Test 5: Insert second message
    print("\n[5] Inserting second HL7 message...")
    hl7_msg2 = r"MSH|^~\\&|EPIC|EPICADT|SMS|SMSADT|20240115130000||ADT^A04|123457|P|2.5\rPID|1||987654321||SMITH^JANE^A||19900501|F|||456 ELM ST^^SOMEWHERE^NY^67890||555-5678|"
    query2 = f"INSERT MESSAGE INTO test_channel VALUES ('{hl7_msg2}')"
    status, response = send_query(HOST, PORT, query2)
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success: {response}")

    # Test 6: Verify both messages
    print("\n[6] Querying both messages...")
    status, response = send_query(HOST, PORT, "SELECT MSG_ID, PID.5 FROM test_channel")
    if status != 0:
        print(f"✗ Failed: {response}")
        return 1
    print(f"✓ Success:")
    print(response)

    print("\n" + "=" * 60)
    print("✓ All tests passed!")
    print("=" * 60)
    print("\nNow please restart the HL7DB server and run:")
    print("  python3 test_persistence_reload.py")
    print("to verify messages are reloaded from disk.")

    return 0


if __name__ == '__main__':
    sys.exit(main())
