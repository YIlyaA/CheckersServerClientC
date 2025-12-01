import socket
import sys

BOARD_SIZE = 8


def parse_board(line: str) -> str:
    parts = line.split(maxsplit=1)
    if len(parts) != 2:
        return ""
    board_str = parts[1]
    board_str = board_str.strip()
    if len(board_str) < 64:
        board_str = board_str.ljust(64, '.')
    return board_str[:64]


def print_board(board_str: str, color: str | None):
    if len(board_str) != 64:
        print("Invalid board length:", len(board_str))
        return

    board = []
    idx = 0
    for r in range(BOARD_SIZE):
        row = []
        for c in range(BOARD_SIZE):
            row.append(board_str[idx])
            idx += 1
        board.append(row)

    pov = color

    print()
    print("    0 1 2 3 4 5 6 7 ")
    print("   -----------------")

    for vr in range(BOARD_SIZE):
        if pov == "BLACK":
            r = BOARD_SIZE - 1 - vr
        else:
            r = vr

        print(f"{vr} | ", end="")
        for vc in range(BOARD_SIZE):
            if pov == "BLACK":
                c = BOARD_SIZE - 1 - vc
            else:
                c = vc

            ch = board[r][c]

            if (r + c) % 2 == 0 and ch == '.':
                ch = ' '

            print(ch, end=" ")
        print("|")
    print("   -----------------")
    print()


def main():
    if len(sys.argv) < 3:
        print("Usage: python client.py <host> <port>")
        print("Example: python client.py 127.0.0.1 1100")
        return

    host = sys.argv[1]
    port = int(sys.argv[2])

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    print(f"Connected to {host}:{port}")

    f = sock.makefile("r", encoding="utf-8", newline="\n")

    my_color = None
    last_board = None
    running = True

    while running:
        line = f.readline()
        if not line:
            print("Server closed connection.")
            break

        line = line.strip()
        if not line:
            continue

        if line.startswith("WELCOME"):
            parts = line.split()
            if len(parts) == 2:
                my_color = parts[1]
                print(f"Your color: {my_color}")

        elif line.startswith("BOARD"):
            last_board = parse_board(line)
            print_board(last_board, my_color)

        elif line in ("WAITING_FOR_OPPONENT",):
            print("Waiting for second player...")

        elif line in ("YOUR_TURN", "YOUR_TURN_CONTINUE_CAPTURE"):
            print("Your move!")

            while True:
                user_inp = input("Enter move as 'r1 c1 r2 c2' or 'quit': ").strip()
                if user_inp.lower() in ("q", "quit", "exit"):
                    sock.sendall(b"QUIT\n")
                    running = False
                    break

                parts = user_inp.split()
                if len(parts) != 4:
                    print("Invalid format. Need 4 numbers.")
                    continue

                try:
                    r1, c1, r2, c2 = map(int, parts)
                except ValueError:
                    print("All coordinates must be integers.")
                    continue

                if my_color == "BLACK":
                    sr1 = BOARD_SIZE - 1 - r1
                    sc1 = BOARD_SIZE - 1 - c1
                    sr2 = BOARD_SIZE - 1 - r2
                    sc2 = BOARD_SIZE - 1 - c2
                else:
                    sr1, sc1, sr2, sc2 = r1, c1, r2, c2

                msg = f"MOVE {sr1} {sc1} {sr2} {sc2}\n"
                sock.sendall(msg.encode("utf-8"))

                break

        elif line in ("OPP_TURN", "OPP_TURN_CAPTURE_CHAIN"):
            print("Waiting for opponent move...")

        elif line == "MOVE_INVALID":
            print("Server: move invalid.")

        elif line == "MOVE_OK":
            print("Move accepted.")

        elif line == "OPPONENT_MOVED":
            print("Opponent made a move.")

        elif line == "YOU_WIN":
            print("=== YOU WIN! ===")
            running = False

        elif line == "YOU_LOSE":
            print("=== YOU LOSE ===")
            running = False

        elif line == "DRAW":
            print("=== DRAW ===")
            running = False

        elif line == "OPPONENT_LEFT":
            print("Opponent left the game.")
            running = False

        elif line.startswith("ERROR_"):
            print("Error from server:", line)

    sock.close()


if __name__ == "__main__":
    main()
