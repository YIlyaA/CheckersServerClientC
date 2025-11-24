import socket
import sys

HOST = '127.0.0.1'
PORT = 1100

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))

    print("Connected to server")

    while True:
        data = s.recv(1024)
        if not data:
            print("Server closed connection")
            break

        msg = data.decode().strip()
        print("SERVER:", msg)

        if msg in ("YOUR_TURN",):
            # пора ходить
            cmd = input("Enter move: MOVE from to or QUIT -> ")
            s.sendall((cmd + "\n").encode())

        if msg in ("WIN", "LOSE", "DRAW", "OPPONENT_LEFT"):
            print("Game ended:", msg)
            break

    s.close()

if __name__ == "__main__":
    main()
