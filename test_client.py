#!/usr/bin/env python3
"""
Simple test client for HL7DB TCP server
"""

import socket
import struct

def send_query(sock, query):
    """Send a query and receive the response"""
    # Send request: [4 bytes length][query string]
    query_bytes = query.encode('utf-8')
    length = len(query_bytes)

    # Pack length as big-endian 32-bit integer
    sock.sendall(struct.pack('!I', length))
    sock.sendall(query_bytes)

    # Receive response: [4 bytes status][4 bytes length][result data]
    status_bytes = sock.recv(4)
    if len(status_bytes) != 4:
        raise Exception("Connection closed")

    status = struct.unpack('!I', status_bytes)[0]

    length_bytes = sock.recv(4)
    if len(length_bytes) != 4:
        raise Exception("Connection closed")

    result_length = struct.unpack('!I', length_bytes)[0]

    # Read result data
    result_data = b''
    while len(result_data) < result_length:
        chunk = sock.recv(result_length - len(result_data))
        if not chunk:
            raise Exception("Connection closed")
        result_data += chunk

    result = result_data.decode('utf-8')

    return status, result

def main():
    # Connect to HL7DB server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 7777))

    print("Connected to HL7DB server on localhost:7777\n")

    # Test 1: Create a channel
    print("Test 1: CREATE CHANNEL adt_messages")
    status, result = send_query(sock, "CREATE CHANNEL adt_messages")
    print(f"Status: {status}")
    print(f"Result: {result}")
    print()

    # Test 2: Insert a message
    hl7_message = "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615083000||ADT^A01|MSG00001|P|2.5\\rPID|1||123456789^^^FACILITY^MR||JONES^WILLIAM^A||19800515|M"

    print("Test 2: INSERT MESSAGE")
    query = f"INSERT MESSAGE INTO adt_messages '{hl7_message}'"
    status, result = send_query(sock, query)
    print(f"Status: {status}")
    print(f"Result: {result}")
    print()

    # Test 3: Insert another message
    hl7_message2 = "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615090000||ADT^A01|MSG00002|P|2.5\\rPID|1||987654321^^^FACILITY^MR||DOE^JANE^M||19750310|F"

    print("Test 3: INSERT second message")
    query = f"INSERT MESSAGE INTO adt_messages '{hl7_message2}'"
    status, result = send_query(sock, query)
    print(f"Status: {status}")
    print(f"Result: {result}")
    print()

    # Test 4: SELECT all messages
    print("Test 4: SELECT * FROM adt_messages")
    status, result = send_query(sock, "SELECT * FROM adt_messages")
    print(f"Status: {status}")
    print(f"Result:\n{result}")
    print()

    # Test 5: SELECT with WHERE clause
    print("Test 5: SELECT with WHERE clause")
    status, result = send_query(sock, "SELECT * FROM adt_messages WHERE segment.PID[5] = 'JONES'")
    print(f"Status: {status}")
    print(f"Result:\n{result}")
    print()

    # Test 6: POP MESSAGE (FIFO queue)
    print("Test 6: POP MESSAGE FROM adt_messages")
    status, result = send_query(sock, "POP MESSAGE FROM adt_messages")
    print(f"Status: {status}")
    print(f"Result:\n{result}")
    print()

    # Test 7: Verify one message left
    print("Test 7: SELECT * FROM adt_messages (after POP)")
    status, result = send_query(sock, "SELECT * FROM adt_messages")
    print(f"Status: {status}")
    print(f"Result:\n{result}")
    print()

    sock.close()
    print("Tests completed successfully!")

if __name__ == '__main__':
    main()
