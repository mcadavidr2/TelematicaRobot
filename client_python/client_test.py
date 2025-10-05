import socket

HOST = "127.0.0.1"
PORT = 8000

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))
print("Conectado al servidor de prueba.")

msg = "AUTH admin 1234\n"
print("DEBUG: enviando repr:", repr(msg))
sock.sendall(msg.encode('utf-8'))

resp = sock.recv(2048)
print("DEBUG: recibido repr:", repr(resp.decode('utf-8', errors='replace')))

sock.close()
