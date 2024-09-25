#include <raylib.h>
#include <iostream>
using namespace std;

const int screenWidth = 800;
const int screenHeight = 850;
const int gridSize = 8;
const int boardSize = 800;
const int cellSize = boardSize / gridSize;
const int maxPieces = 24;

struct Piece {
    int x, y;
    bool isKing;
    bool isWhite;
    bool isCaptured;
};

Piece pieces[maxPieces * 2];
int pieceCount = 0;

int selectedPieceIndex = -1;
bool isWhiteTurn = true;
bool gameOver = false;
bool playerIsWhite = true;  // Player's chosen color
string winnerMessage;
bool gameStarted = false;

// Variables to count how many pieces each player has captured
int whiteCapturedCount = 0;
int blackCapturedCount = 0;

void InitPieces() {
    pieceCount = 0;
    whiteCapturedCount = 0;  // Reset capture counts
    blackCapturedCount = 0;

    int whiteStartY = playerIsWhite ? 0 : 5;
    int blackStartY = playerIsWhite ? 5 : 0;

    // Initialize white pieces (player's pieces)
    for (int y = whiteStartY; y < whiteStartY + 3; y++) {
        for (int x = (y % 2 == 0) ? 0 : 1; x < gridSize; x += 2) {
            pieces[pieceCount++] = {x, y, false, playerIsWhite, false};
        }
    }

    // Initialize black pieces (opponent's pieces)
    for (int y = blackStartY; y < blackStartY + 3; y++) {
        for (int x = (y % 2 == 0) ? 0 : 1; x < gridSize; x += 2) {
            pieces[pieceCount++] = {x, y, false, !playerIsWhite, false};
        }
    }
}

void DrawBoard() {
    Color darkWood = {101, 67, 33, 255};
    Color lightWood = {222, 184, 135, 255};

    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            Color color = (x + y) % 2 == 0 ? lightWood : darkWood;
            DrawRectangle(x * cellSize + (screenWidth - boardSize) / 2, y * cellSize + 50, cellSize, cellSize, color);
        }
    }

    // Display the number of pieces each player has captured
    string whiteCapturedText = "White captured: " + to_string(whiteCapturedCount);
    string blackCapturedText = "Black captured: " + to_string(blackCapturedCount);
    DrawText(whiteCapturedText.c_str(), 20, 10, 20, DARKGRAY);
    DrawText(blackCapturedText.c_str(), screenWidth - 200, 10, 20, DARKGRAY);
}

void DrawPieces() {
    for (int i = 0; i < pieceCount; i++) {
        if (!pieces[i].isCaptured) {
            Color color = pieces[i].isWhite ? WHITE : BLACK;
            int pieceCenterX = pieces[i].x * cellSize + cellSize / 2 + (screenWidth - boardSize) / 2;
            int pieceCenterY = pieces[i].y * cellSize + cellSize / 2 + 50;
            int radius = cellSize / 2 - 10;

            // Draw shadow (slightly offset down and right)
            DrawCircle(pieceCenterX + 5, pieceCenterY + 5, radius, DARKGRAY);

            // Draw main piece
            DrawCircle(pieceCenterX, pieceCenterY, radius, color);

            // Draw top highlight to simulate light source
            DrawCircle(pieceCenterX, pieceCenterY - 3, radius - 5, Fade(WHITE, 0.5));

            // Draw darker lower edge to enhance the 3D effect
            DrawCircleLines(pieceCenterX, pieceCenterY, radius, Fade(BLACK, 0.3));

            // Highlight the selected piece with a glowing effect
            if (i == selectedPieceIndex) {
                DrawCircle(pieceCenterX, pieceCenterY, radius + 10, Fade(YELLOW, 0.3)); // Outer glow
                DrawCircleLines(pieceCenterX, pieceCenterY, radius + 5, YELLOW);         // Inner outline
            }

            // Draw crown if the piece is a king
            if (pieces[i].isKing) {
                DrawCircle(pieceCenterX, pieceCenterY, radius / 4, GOLD);
            }
        }
    }
}

int GetPieceIndexAt(int x, int y) {
    for (int i = 0; i < pieceCount; i++) {
        if (!pieces[i].isCaptured && pieces[i].x == x && pieces[i].y == y) {
            return i;
        }
    }
    return -1;
}

bool IsMoveValid(Piece piece, int x, int y) {
    if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) return false;
    if (abs(piece.x - x) != abs(piece.y - y)) return false;

    // Check if the destination square is occupied by another piece
    if (GetPieceIndexAt(x, y) != -1) return false;

    if (piece.isKing) {
        int dx = (x > piece.x) ? 1 : -1;
        int dy = (y > piece.y) ? 1 : -1;
        int consecutivePieces = 0;

        for (int i = 1; i < abs(x - piece.x); i++) {
            int px = piece.x + i * dx;
            int py = piece.y + i * dy;
            int index = GetPieceIndexAt(px, py);
            if (index != -1) {
                consecutivePieces++;
                // If there are two consecutive pieces in the path, move is invalid
                if (consecutivePieces > 1) return false;
            }
        }
    } else {
        if (abs(piece.x - x) != 1) return false;
        if ((piece.isWhite && y <= piece.y) || (!piece.isWhite && y >= piece.y)) return false;
    }

    return true;
}

bool IsCaptureValid(Piece piece, int x, int y) {
    if (piece.isKing) {
        int dx = (x > piece.x) ? 1 : -1;
        int dy = (y > piece.y) ? 1 : -1;
        int dist = abs(x - piece.x);
        int opponentCount = 0;
        for (int i = 1; i < dist; i++) {
            int px = piece.x + i * dx;
            int py = piece.y + i * dy;
            int index = GetPieceIndexAt(px, py);
            if (index != -1) {
                if (pieces[index].isWhite != piece.isWhite && !pieces[index].isCaptured) {
                    opponentCount++;
                    if (opponentCount > 1) return false;
                } else {
                    return false;
                }
            }
        }
        if (GetPieceIndexAt(x, y) != -1) return false;
        return opponentCount == 1;
    } else {
        if ((piece.isWhite && y <= piece.y) || (!piece.isWhite && y >= piece.y)) return false; // Forward movement only
        if (abs(piece.x - x) != 2 || abs(piece.y - y) != 2) return false;

        int midX = (piece.x + x) / 2;
        int midY = (piece.y + y) / 2;

        for (int i = 0; i < pieceCount; i++) {
            if (!pieces[i].isCaptured && pieces[i].x == midX && pieces[i].y == midY && pieces[i].isWhite != piece.isWhite) {
                if (GetPieceIndexAt(x, y) != -1) return false;
                return true;
            }
        }
    }
    return false;
}

bool CanPieceCaptureAgain(int index) {
    Piece piece = pieces[index];
    int dx[] = {-2, 2}; // Capture move is always 2 cells away horizontally
    int dy[] = {-2, 2}; // Capture move is always 2 cells away vertically

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            int newX = piece.x + dx[i];
            int newY = piece.y + dy[j];
            
            // Check if the capture move is valid
            if (newX >= 0 && newX < gridSize && newY >= 0 && newY < gridSize && IsCaptureValid(piece, newX, newY)) {
                return true; // If any valid capture exists, return true
            }
        }
    }
    return false; // No valid capture exists
}


void CapturePiece(Piece piece, int x, int y) {
    if (piece.isKing) {
        int dx = (x > piece.x) ? 1 : -1;
        int dy = (y > piece.y) ? 1 : -1;
        int dist = abs(x - piece.x);

        for (int i = 1; i < dist; i++) {
            int px = piece.x + i * dx;
            int py = piece.y + i * dy;
            int index = GetPieceIndexAt(px, py);
            if (index != -1 && pieces[index].isWhite != piece.isWhite) {
                pieces[index].isCaptured = true;

                // Increment captured count for the player who made the capture
                if (piece.isWhite) {
                    whiteCapturedCount++;  // White captures
                } else {
                    blackCapturedCount++;  // Black captures
                }
                break;
            }
        }
    } else {
        int midX = (piece.x + x) / 2;
        int midY = (piece.y + y) / 2;

        for (int i = 0; i < pieceCount; i++) {
            if (!pieces[i].isCaptured && pieces[i].x == midX && pieces[i].y == midY) {
                pieces[i].isCaptured = true;

                // Increment captured count for the player who made the capture
                if (piece.isWhite) {
                    whiteCapturedCount++;  // White captures
                } else {
                    blackCapturedCount++;  // Black captures
                }
            }
        }
    }
}

void MovePiece(int index, int x, int y) {
    bool captured = IsCaptureValid(pieces[index], x, y);
    if (captured) {
        CapturePiece(pieces[index], x, y);
    }

    pieces[index].x = x;
    pieces[index].y = y;

    // Promote piece to king if it reaches the opposite side
    if ((pieces[index].isWhite && y == gridSize - 1) || (!pieces[index].isWhite && y == 0)) {
        pieces[index].isKing = true;
    }

    // Check if the piece can capture again after a capture
    if (captured && CanPieceCaptureAgain(index)) {
        // Keep the piece selected for another capture move
        selectedPieceIndex = index;
    } else {
        // No more
        // No more captures possible, switch turn
        isWhiteTurn = !isWhiteTurn;
        selectedPieceIndex = -1;  // Deselect the piece
    }
}

bool HasValidMoves(bool isWhite) {
    for (int i = 0; i < pieceCount; i++) {
        if (pieces[i].isWhite == isWhite && !pieces[i].isCaptured) {
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = -2; dy <= 2; dy++) {
                    int newX = pieces[i].x + dx;
                    int newY = pieces[i].y + dy;
                    if (IsMoveValid(pieces[i], newX, newY) || IsCaptureValid(pieces[i], newX, newY)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void CheckForWin() {
    if (!HasValidMoves(isWhiteTurn)) {
        gameOver = true;
        winnerMessage = isWhiteTurn ? "Black Wins!" : "White Wins!";
    }
}

void UpdateGame() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !gameOver) {
        Vector2 mousePos = GetMousePosition();
        int x = (mousePos.x - (screenWidth - boardSize) / 2) / cellSize;
        int y = (mousePos.y - 50) / cellSize;

        int clickedPieceIndex = GetPieceIndexAt(x, y);

        if (selectedPieceIndex == -1) {
            // Select a piece if none is currently selected
            if (clickedPieceIndex != -1 && pieces[clickedPieceIndex].isWhite == isWhiteTurn) {
                selectedPieceIndex = clickedPieceIndex;
            }
        } else {
            // If a piece is selected, either move it, capture, or deselect it
            if (clickedPieceIndex == selectedPieceIndex) {
                // Deselect if clicking the selected piece again
                selectedPieceIndex = -1;
            } else if (clickedPieceIndex != -1 && pieces[clickedPieceIndex].isWhite == isWhiteTurn) {
                // Switch selection to another piece
                selectedPieceIndex = clickedPieceIndex;
            } else if (IsMoveValid(pieces[selectedPieceIndex], x, y) || IsCaptureValid(pieces[selectedPieceIndex], x, y)) {
                // Move the selected piece
                MovePiece(selectedPieceIndex, x, y);
                CheckForWin();
            } else {
                // Deselect if clicking an invalid move
                selectedPieceIndex = -1;
            }
        }
    }
}

void DrawMenu() {
    ClearBackground(RAYWHITE);
    DrawText("Choose your color:", screenWidth / 2 - MeasureText("Choose your color:", 30) / 2, screenHeight / 2 - 100, 30, DARKGRAY);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        Rectangle whiteButton = { screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 50 };
        Rectangle blackButton = { screenWidth / 2 - 100, screenHeight / 2 + 10, 200, 50 };

        if (CheckCollisionPointRec(mousePos, whiteButton)) {
            playerIsWhite = true;
            isWhiteTurn = true;  // White starts the game
            gameStarted = true;
            InitPieces();  // Initialize pieces with the chosen color
        } else if (CheckCollisionPointRec(mousePos, blackButton)) {
            playerIsWhite = false;
            isWhiteTurn = false;  // Black starts the game
            gameStarted = true;
            InitPieces();  // Initialize pieces with the chosen color
        }
    }

    DrawRectangle(screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 50, LIGHTGRAY);
    DrawRectangleLines(screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 50, DARKGRAY);
    DrawText("Play as White", screenWidth / 2 - MeasureText("Play as White", 20) / 2, screenHeight / 2 - 30, 20, DARKGRAY);

    DrawRectangle(screenWidth / 2 - 100, screenHeight / 2 + 10, 200, 50, LIGHTGRAY);
    DrawRectangleLines(screenWidth / 2 - 100, screenHeight / 2 + 10, 200, 50, DARKGRAY);
    DrawText("Play as Black", screenWidth / 2 - MeasureText("Play as Black", 20) / 2, screenHeight / 2 + 30, 20, DARKGRAY);
}

int main() {
    InitWindow(screenWidth, screenHeight, "Checkers");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (!gameStarted) {
            DrawMenu();
        } else {
            if (!gameOver) {
                UpdateGame();
            }
            ClearBackground(RAYWHITE);

            string currentPlayer = isWhiteTurn ? "White's Turn" : "Black's Turn";
            DrawText(currentPlayer.c_str(), screenWidth / 2 - MeasureText(currentPlayer.c_str(), 30) / 2, 10, 30, DARKGRAY);

            DrawBoard();
            DrawPieces();

            if (gameOver) {
                DrawText(winnerMessage.c_str(), screenWidth / 2 - MeasureText(winnerMessage.c_str(), 60) / 2, screenHeight / 2 - 30, 60, GREEN);
            }
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
