import socket, threading

HOST = "127.0.0.1"
PORT = 8000

def recibir(sock):
    while True:
        try:
            data = sock.recv(2048).decode('utf-8', errors='replace')
            if not data:
                break
            print("[Servidor]", data.strip())
        except Exception as e:
            print("Error en recv:", e)
            break

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))
print("Conectado al servidor")

t = threading.Thread(target=recibir, args=(sock,), daemon=True)
t.start()

while True:
    cmd = input(">>> ")
    if cmd.lower() in ("exit", "quit"):
        break
    sock.sendall((cmd + "\n").encode('utf-8'))

sock.close()
