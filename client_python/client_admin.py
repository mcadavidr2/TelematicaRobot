import socket
import threading

HOST = "127.0.0.1"
PORT = 8000

def recibir(sock):
    while True:
        try:
            data = sock.recv(1024).decode("utf-8")
            if not data:
                break
            print(f"[Servidor] {data.strip()}")
        except:
            break

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    print("Conectado al servidor.")

    # Hilo para escuchar respuestas del servidor
    hilo = threading.Thread(target=recibir, args=(sock,), daemon=True)
    hilo.start()

    # Loop para enviar comandos
    while True:
        msg = input(">>> ")  # escribes AUTH, SET, LIST, etc.
        if msg.lower() == "exit":
            break
        sock.sendall(msg.encode("utf-8"))

    sock.close()

if __name__ == "__main__":
    main()
