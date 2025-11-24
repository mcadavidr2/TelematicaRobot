import asyncio
import websockets
import socket
import threading

SERVER_IP = "44.197.195.18"  
SERVER_PORT = 8000

WS_PORT = 8765   


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER_IP, SERVER_PORT))
print("Conectado al servidor C en EC2")

clients_web = set()

def tcp_listener():
    """Hilo que escucha mensajes del servidor C y los reenvía a WebSocket."""
    while True:
        try:
            data = sock.recv(2048).decode("utf-8", errors="ignore")
            if not data:
                continue
            print("[EC2] ", data.strip())

           
            asyncio.run(broadcast_ws(data.strip()))
        except Exception as e:
            print("Error TCP:", e)
            break

threading.Thread(target=tcp_listener, daemon=True).start()



async def broadcast_ws(message):
    """Enviar a todos los navegadores conectados."""
    dead = []
    for ws in clients_web:
        try:
            await ws.send(message)
        except:
            dead.append(ws)
    for ws in dead:
        clients_web.remove(ws)


async def ws_handler(websocket):
    print("Web conectado")
    clients_web.add(websocket)

    try:
        async for msg in websocket:
            print("[WEB] ", msg)
            sock.send((msg + "\n").encode("utf-8"))
    finally:
        clients_web.remove(websocket)


async def main():
    async with websockets.serve(ws_handler, "0.0.0.0", WS_PORT):
        print(f"WebSocket listo en ws://0.0.0.0:{WS_PORT}")
        await asyncio.Future()  


if __name__ == "__main__":
    asyncio.run(main())
