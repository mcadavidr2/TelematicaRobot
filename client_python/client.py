import tkinter as tk
import threading
import socket

GRID_SIZE = 11
CELL_SIZE = 30

class RobotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot en plano cartesiano")

        # Canvas para dibujar la grilla
        self.canvas = tk.Canvas(
            root, width=GRID_SIZE*CELL_SIZE, height=GRID_SIZE*CELL_SIZE, bg="white"
        )
        self.canvas.pack()

        # Dibujar grilla
        for i in range(GRID_SIZE+1):
            self.canvas.create_line(i*CELL_SIZE, 0, i*CELL_SIZE, GRID_SIZE*CELL_SIZE, fill="gray")
            self.canvas.create_line(0, i*CELL_SIZE, GRID_SIZE*CELL_SIZE, i*CELL_SIZE, fill="gray")

        # Posición inicial del robot
        self.robot_x = 5
        self.robot_y = 5
        self.robot = self.draw_robot(self.robot_x, self.robot_y)

    def draw_robot(self, x, y):
        """Dibuja el robot como un círculo rojo en la celda (x,y)."""
        x1 = x * CELL_SIZE + 5
        y1 = y * CELL_SIZE + 5
        x2 = (x+1) * CELL_SIZE - 5
        y2 = (y+1) * CELL_SIZE - 5
        return self.canvas.create_oval(x1, y1, x2, y2, fill="red")

    def update_robot(self, new_x, new_y):
        """Mueve el robot a coordenadas absolutas (x,y)."""
        if 0 <= new_x < GRID_SIZE and 0 <= new_y < GRID_SIZE:
            self.robot_x = new_x
            self.robot_y = new_y
            self.canvas.delete(self.robot)
            self.robot = self.draw_robot(self.robot_x, self.robot_y)

    def move_robot(self, dx, dy):
        """Mueve el robot de forma relativa (dx,dy)."""
        self.update_robot(self.robot_x + dx, self.robot_y + dy)


import tkinter as tk
import threading
import socket

GRID_SIZE = 11
CELL_SIZE = 30

class RobotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot en plano cartesiano")

        # Canvas para dibujar la grilla
        self.canvas = tk.Canvas(
            root, width=GRID_SIZE*CELL_SIZE, height=GRID_SIZE*CELL_SIZE, bg="white"
        )
        self.canvas.pack()

        # Dibujar grilla
        for i in range(GRID_SIZE+1):
            self.canvas.create_line(i*CELL_SIZE, 0, i*CELL_SIZE, GRID_SIZE*CELL_SIZE, fill="gray")
            self.canvas.create_line(0, i*CELL_SIZE, GRID_SIZE*CELL_SIZE, i*CELL_SIZE, fill="gray")

        # Posición inicial del robot
        self.robot_x = 5
        self.robot_y = 5
        self.robot = self.draw_robot(self.robot_x, self.robot_y)

    def draw_robot(self, x, y):
        """Dibuja el robot como un círculo rojo en la celda (x,y)."""
        x1 = x * CELL_SIZE + 5
        y1 = y * CELL_SIZE + 5
        x2 = (x+1) * CELL_SIZE - 5
        y2 = (y+1) * CELL_SIZE - 5
        return self.canvas.create_oval(x1, y1, x2, y2, fill="red")

    def update_robot(self, new_x, new_y):
        """Mueve el robot a coordenadas absolutas (x,y)."""
        if 0 <= new_x < GRID_SIZE and 0 <= new_y < GRID_SIZE:
            self.robot_x = new_x
            self.robot_y = new_y
            self.canvas.delete(self.robot)
            self.robot = self.draw_robot(self.robot_x, self.robot_y)

    def move_robot(self, dx, dy):
        """Mueve el robot de forma relativa (dx,dy)."""
        self.update_robot(self.robot_x + dx, self.robot_y + dy)


def listen_server(sock, gui):
    while True:
        try:
            data = sock.recv(1024).decode("utf-8").strip()
            if not data:
                break
            print("[DEBUG recibido]", repr(data))  # ahora siempre ves lo que llega

            if data.startswith("MOVED"):
                # Mensaje tipo: MOVED X=3 Y=7
                parts = data.split()
                x = int(parts[1].split("=")[1])
                y = int(parts[2].split("=")[1])
                gui.root.after(0, lambda: gui.update_robot(x, y))

            else:
                print("[RESPUESTA SERVER]", data)

        except Exception as e:
            print("Error en socket:", e)
            break




if __name__ == "__main__":
    root = tk.Tk()
    gui = RobotGUI(root)

    HOST = "127.0.0.1"
    PORT = 8000
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    print("Conectado al servidor")

    # Hilo separado para escuchar al servidor
    threading.Thread(target=listen_server, args=(sock, gui), daemon=True).start()

    try:
        root.mainloop()
    finally:
        sock.close()




if __name__ == "__main__":
    root = tk.Tk()
    gui = RobotGUI(root)

    HOST = "127.0.0.1"
    PORT = 8000
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    print("Conectado al servidor")

    # Hilo separado para escuchar al servidor
    threading.Thread(target=listen_server, args=(sock, gui), daemon=True).start()

    try:
        root.mainloop()
    finally:
        sock.close()
