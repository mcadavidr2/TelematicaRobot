import socket
import threading
import time

HOST = "3.85.241.80"
PORT = 8000

def recibir(sock):
    while True:
        try:
            data = sock.recv(2048).decode('utf-8', errors='replace')
            if not data:
                print("[Servidor] Conexión cerrada")
                break
            print("[Servidor]", data.strip())
        except Exception as e:
            print("Error en recv:", e)
            break

# ==== Conection con reintento ====
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

print("Conectando a servidor admin...")
time.sleep(0.4)  # evita que el servidor te bote al instante

sock.connect((HOST, PORT))
print("Conectado al servidor ADMIN")

t = threading.Thread(target=recibir, args=(sock,), daemon=True)
t.start()

while True:
    cmd = input(">>> ")
    if cmd.lower() in ("exit", "quit"):
        break
    
    if cmd.strip() == "":
        continue

    sock.sendall((cmd + "\n").encode('utf-8'))

sock.close()


