# Architecture Direction

This document defines the intended architectural shape for the codebase. It is intentionally high level and will be expanded as subsystems are implemented.

## Primitive Types

All code should use the project's own sized types from `src/types.h` instead of built-in or CRT types wherever bit width is meaningful:

| Type | Width | Use for |
|---|---|---|
| `int8` / `uint8` | 8-bit | Bytes, small flags, character data |
| `int16` / `uint16` | 16-bit | Compact values where range is known |
| `int32` / `uint32` | 32-bit | General integers, indices, counts |
| `int64` / `uint64` | 64-bit | Large counts, timestamps, sizes, offsets |

`uint64` replaces `size_t` for all sizes and offsets — a `static_assert` in `types.h` enforces that the two are the same width. `bool` and `void*` are used as normal. Do not use `int`, `unsigned`, `char`, `long`, `size_t`, or `__int*` types directly outside of `types.h` itself.

## Top-Level Principles

- Prefer simple modules with C-style function boundaries.
- Keep state explicit and visible in plain structs.
- Allocate long-lived memory at startup.
- Divide memory into distinct arenas based on lifetime and subsystem ownership.
- Avoid hidden global behavior unless the platform boundary requires it.
- Keep the design legible to future agents through small files and explicit naming.
- `windows.h` and other Win32 headers are required by the platform and are always allowed. "Avoid the C standard library" targets CRT headers like `<stdio.h>`, `<stdlib.h>`, and `<string.h>`, not the Win32 API.
- Static `const` data at file scope (lookup tables, named constants) is fine and preferred over pushing compile-time data into memory arenas.

## Planned Subsystems

### Platform

Own the Win32 application entry point, window creation, message pump, mouse input capture, timing, and file access needed by the application.

### Software Renderer

Own the pixel buffer, drawing primitives, sprite or glyph composition strategy, and presentation to the window surface using the Windows API.

The renderer is implemented in `src/renderer.h` and `src/renderer.cpp`. Its state lives in `RendererState`:

| Field | Type | Purpose |
|---|---|---|
| `pixels` | `Pixel*` | CPU-side BGRA pixel buffer; allocated from the `renderer` arena at startup |
| `width` / `height` | `int32` | Logical render resolution (1280 × 720 by default) |
| `bmi` | `BITMAPINFO` | Filled once at startup; passed to `StretchDIBits` every frame |

The public API:

| Function | Purpose |
|---|---|
| `RendererInit(rs, arena, w, h)` | Push the pixel buffer from the arena, fill `bmi` |
| `ClearBuffer(rs, color)` | Fill every pixel with a solid `Pixel` colour |
| `DrawRect(rs, x, y, w, h, color)` | Fill an axis-aligned rectangle; silently clips to buffer bounds |
| `DrawFilledCircle(rs, cx, cy, radius, color)` | Fill a circle using integer-only midpoint arithmetic; silently clips to buffer bounds |
| `PresentFrame(rs, window)` | Call `StretchDIBits` to blit the pixel buffer to the window client area |

`PresentFrame` acquires the DC with `GetDC`/`ReleaseDC` each frame — no per-frame GDI objects are allocated. The pixel format is BGRA with 32 bits per pixel and a top-down row order (negative `biHeight`).

### Memory

Owns startup allocation and arena partitioning. One 64 MB block is reserved and committed via `VirtualAlloc` at process startup. It is carved into four contiguous arenas:

| Arena | Size | Purpose |
|---|---|---|
| `game_state` | 16 MB | Board state, turn state, move history, win state |
| `renderer` | 16 MB | Pixel buffer and drawing scratch |
| `input` | 16 MB | Input state and drag-and-drop scratch |
| `test_scratch` | 16 MB | Temporary storage for in-process tests; reset between test cases |

All arenas share the same `Arena` type: `{ base, size, offset }`. Push helpers advance `offset` and return an aligned pointer. Individual frees are not supported. The `test_scratch` arena is reset via `ArenaReset` between test cases. Additional arenas should only be added when a subsystem has a clear and distinct lifetime boundary.

The implementation lives in `src/memory.h`.

### Game State

Own board state, turn state, move history needed for rule evaluation, win state, and restart flow.

The board is represented as a flat `Board` struct (declared in `src/board.h`) containing an 8×8 array of `Square` values. Each `Square` stores a `PieceType` and a `Color` — both `uint8` enums. `PIECE_NONE` / `COLOR_NONE` identify an empty square.

Indexing convention: `squares[rank][file]` where rank 0 is White's back rank (rank 1 in chess notation) and rank 7 is Black's back rank (rank 8). Files 0–7 map to a–h.

`InitBoard(Board*)` sets up the standard starting position. The caller is responsible for pushing a `Board` from the `game_state` arena before calling it.

#### Game Restart Flow

`InputRestart(InputState* input, GameState* gs)` (declared in `src/input.h`, implemented in `src/input.cpp`) is the single entry point for resetting a finished or in-progress game:

1. `InputCancelDrag(input)` — clears any active drag or pending promotion before touching game state, so no stale cursor or move-list data survives the reset.
2. `InitGameState(gs)` — resets the board to the standard starting position, sets `side_to_move` to `COLOR_WHITE`, clears en-passant targets, and restores all four castling rights.

`InputRestart` is safe to call at any point, including during an active drag or while a promotion picker is visible.  The caller (`src/main.cpp`) additionally resets the `game_state` and `input` arena offsets to zero via `ArenaReset` before calling `InputRestart`; no re-allocation is needed because both `g_GameState` and `g_InputState` were pushed from offset 0 of their respective arenas at startup and remain valid after the reset.

The platform layer (`src/main.cpp`) tracks the current result in `g_GameResult` (a `GameResult` value evaluated every render frame via `EvaluatePosition`).  When `g_GameResult != GAME_ONGOING`, `WM_LBUTTONDOWN` is rerouted exclusively to the restart-button hit test; normal drag and promotion handling are suppressed until the restart occurs.

### Chess Rules

Own legal move generation, check detection, checkmate detection, and any supporting rule validation.

#### Move Representation

`src/moves.h` defines the core types used for move generation and application:

- **`Move`** — a single candidate move. Fields: `from_rank`, `from_file`, `to_rank`, `to_file`, `promotion` (`PIECE_NONE` = normal move; `PIECE_QUEEN`/`PIECE_ROOK`/`PIECE_BISHOP`/`PIECE_KNIGHT` = the chosen promotion piece), `is_en_passant`, `is_castling` (when true, `ApplyMove` also repositions the rook).
- **`MoveList`** — a fixed-size array of up to `MAX_MOVES_PER_POSITION` (256) `Move` values plus a `count`. Callers zero-initialize and pass a pointer; generator functions append without clearing.
- **`GameState`** — all mutable state needed between moves: `Board board`, `Color side_to_move`, `int8 en_passant_rank` / `int8 en_passant_file` (-1 when no en passant is available), and four castling-rights flags: `castling_white_kingside`, `castling_white_queenside`, `castling_black_kingside`, `castling_black_queenside`. All four are `true` after `InitGameState` and are cleared by `ApplyMove` when the relevant king or rook moves or when a rook is captured on its starting square.

#### Pawn Move Generation

`GeneratePawnMoves(const GameState*, MoveList*)` appends all candidate pawn moves for `gs->side_to_move`:

- **Single push** — one square forward if the destination is empty.
- **Double push** — two squares forward from the starting rank (rank 1 for White, rank 6 for Black) when both intervening squares are empty.
- **Diagonal capture** — one square diagonally forward when an enemy piece occupies that square.
- **En passant** — diagonal forward capture onto `(en_passant_rank, en_passant_file)` when set. The `is_en_passant` flag is set on the resulting `Move`.
- **Promotion** — any move that reaches the back rank (rank 7 for White, rank 0 for Black) generates four moves, one per legal promotion piece: queen, rook, bishop, and knight.

#### Knight Move Generation

`GenerateKnightMoves(const GameState*, MoveList*)` appends all candidate knight moves for `gs->side_to_move`:

- **L-shape jumps** — all 8 offsets: (±2, ±1) and (±1, ±2). Knights ignore blocking pieces.
- **Board filtering** — destinations outside the 8×8 grid are discarded.
- **Friendly-piece filtering** — destinations occupied by a piece of `side_to_move` are discarded. Destinations occupied by an enemy piece are legal (capture).

#### Sliding Piece Move Generation

`GenerateRookMoves`, `GenerateBishopMoves`, and `GenerateQueenMoves` all use ray casting shared by the internal `CastRays` helper:

- **Ray directions** — rook uses the four orthogonal directions; bishop uses the four diagonals; queen uses all eight.
- **Ray termination** — each ray walks one square at a time until it leaves the board, hits a friendly piece (ray stops; square excluded), or hits an enemy piece (ray stops; square included as a capture).

`GenerateQueenMoves` is equivalent to running the rook rays and bishop rays in a single pass over the board.

#### Move Application

`ApplyMove(GameState*, const Move*)` mutates the game state:

1. Records the piece being captured (used for castling-rights logic).
2. Clears the en passant target.
3. For en passant captures, removes the captured pawn from `(from_rank, to_file)`.
4. Moves the piece from source to destination.
5. If `promotion != PIECE_NONE`, replaces the pawn with the promoted piece.
6. If `is_castling`, slides the rook from its starting corner to its castled square (f-file for kingside, d-file for queenside).
7. If the move is a double pawn push, sets the en passant target to the skipped square.
8. Clears castling rights: both rights for the moving side when the king moves; the relevant side's right when a rook departs its starting corner; the relevant side's right when a rook is captured on its starting corner.
9. Advances `side_to_move`.

#### Check Detection

`IsInCheck(const Board*, Color)` returns `true` if the king of the given color is attacked by any enemy piece on the current board.

- Scans the board to locate the king for `color`.
- Delegates to `IsSquareAttackedBy` for the king's square and the enemy color.

`IsSquareAttackedBy(const Board*, int8 rank, int8 file, Color attacker)` returns `true` if any piece of `attacker` can pseudo-legally reach `(rank, file)`. Pawn attacks use direct diagonal detection only (no forward pushes). All other pieces are checked via the knight/rook/bishop/queen/king move generators. Pinned pieces are **not** filtered — per FIDE Article 3.8 a piece attacks its normal squares even when constrained from moving there by an absolute pin; this ensures correct check detection, king-move validation, and castling safety.
- Efficient enough to call repeatedly during legal move filtering (no dynamic allocation; uses stack-local `GameState` and `MoveList`).

#### King Move Generation

`GenerateKingMoves(const GameState*, MoveList*)` appends all candidate king moves for `gs->side_to_move`:

- **One-square steps** — all eight directions: orthogonal and diagonal.
- **Board filtering** — destinations outside the 8×8 grid are discarded.
- **Friendly-piece filtering** — destinations occupied by `side_to_move` are discarded. Destinations occupied by an enemy piece are legal (capture).

#### Castling Move Generation

`GenerateCastlingMoves(const GameState*, MoveList*)` appends kingside and/or queenside castling moves for `gs->side_to_move`, subject to all standard castling rules:

- Castling right for that side must be set in `GameState`.
- King must be on its starting square (e1 / e8) and the relevant rook must be on its corner (h-file for kingside, a-file for queenside).
- All squares between king and rook must be empty.
- The king must not start in check, pass through a checked square, or land on a checked square. The start-square check uses the live board; transit and destination checks use a temporary board copy with the king removed from its starting square, so that sliding-piece attacks that are currently masked by the king are correctly detected (e.g., an enemy rook behind the king that would control the transit squares once the king moves).
- Generated castling moves have `is_castling = true`; the king's destination is g-file (kingside) or c-file (queenside).

#### Legal Move Filtering

`GetLegalMoves(const GameState*, MoveList*)` appends all **fully legal** moves for `gs->side_to_move`:

- Generates all pseudo-legal candidate moves by calling `GeneratePawnMoves`, `GenerateKnightMoves`, `GenerateRookMoves`, `GenerateBishopMoves`, `GenerateQueenMoves`, `GenerateKingMoves`, and `GenerateCastlingMoves`.
- Skips any candidate whose destination square contains `PIECE_KING` — king capture is never legal in standard chess and must not appear in the output.
- For each remaining candidate move, applies the move to a stack-local copy of the `GameState`, calls `IsInCheck` on the resulting board for the moving side, and discards the move if the king is in check.
- Handles all pinned-piece cases, king-into-check cases, and castling naturally through the apply-and-test approach.
- No dynamic allocation; all temporaries are stack-local.

#### Position Evaluation

`EvaluatePosition(const GameState*)` determines the end-of-game result after each move:

- Calls `GetLegalMoves` for `gs->side_to_move`.
- If the side to move has at least one legal move, returns `GAME_ONGOING`.
- If there are no legal moves and the side to move is in check (`IsInCheck` returns `true`), returns checkmate: `GAME_BLACK_WINS` when White is mated, `GAME_WHITE_WINS` when Black is mated.
- If there are no legal moves and the king is **not** in check, returns `GAME_DRAW` (stalemate).

`GameResult` is an enum declared in `src/moves.h`:

| Value | Meaning |
|---|---|
| `GAME_ONGOING` | At least one legal move remains; game continues. |
| `GAME_WHITE_WINS` | Black is in checkmate (no legal moves, king in check). |
| `GAME_BLACK_WINS` | White is in checkmate (no legal moves, king in check). |
| `GAME_DRAW` | The side to move has no legal moves and is not in check (stalemate). |

### Input

Own drag-and-drop piece movement state for both players.  No heap allocation; all state lives in the `input` arena.

The input subsystem is implemented in `src/input.h` and `src/input.cpp`.

#### InputState

| Field | Type | Purpose |
|---|---|---|
| `dragging` | `bool` | `true` while the left button is held and a piece is being dragged |
| `drag_from_rank` / `drag_from_file` | `int8` | Source square of the dragged piece; both are `-1` when not dragging |
| `drag_cursor_x` / `drag_cursor_y` | `int32` | Current cursor pixel position; updated every `WM_MOUSEMOVE` |
| `legal_moves` | `MoveList` | Legal moves originating from the drag-source square; pre-computed at drag start; kept alive during `pending_promotion` for use by `InputHandlePromotionClick` |
| `pending_promotion` | `bool` | `true` after a pawn drop on the last rank, while waiting for the player to choose a promotion piece |
| `promo_from_rank` / `promo_from_file` | `int8` | Source square of the pawn awaiting promotion |
| `promo_to_rank` / `promo_to_file` | `int8` | Target square on the promotion rank |

#### Coordinate Mapping

`PixelToSquare(px, py, board_x, board_y, square_size, &rank, &file)` converts a window-client pixel to a board square.  Returns `false` when the pixel falls outside the 8×8 board area.  Rank 0 is White's back rank at the bottom of the screen; rank 7 is Black's back rank at the top.

#### Drag Handling

The drag state machine spans three Win32 messages:

| Message | Function | Behaviour |
|---|---|---|
| `WM_LBUTTONDOWN` | `InputHandleDragStart` (normal) or `InputHandlePromotionClick` (when `pending_promotion`) | Picks up piece if it belongs to the current player, or resolves the promotion picker |
| `WM_MOUSEMOVE` | `InputHandleDragMove` | Update `drag_cursor_x/y`; no-op when not dragging |
| `WM_LBUTTONUP` | `InputHandleDragEnd` | Drop the piece: apply the move if legal and non-promotion; enter `pending_promotion` if the drop is on a promotion rank; otherwise cancel the drag |

#### Promotion Picker

When `InputHandleDragEnd` detects that the drop target is a pawn-promotion square, it does not apply a move immediately.  Instead it sets `pending_promotion = true` and preserves `legal_moves` (which contains all four promotion variants) for use by `InputHandlePromotionClick`.

`InputHandlePromotionClick` maps the next `WM_LBUTTONDOWN` pixel to a picker slot:

| Promoting side | Slot 0 (rank) | Slot 1 | Slot 2 | Slot 3 |
|---|---|---|---|---|
| White | rank 7 — Queen | rank 6 — Rook | rank 5 — Bishop | rank 4 — Knight |
| Black | rank 0 — Queen | rank 1 — Rook | rank 2 — Bishop | rank 3 — Knight |

A click outside the picker column or below/above the four slots cancels the pending promotion without changing the board.

`InputCancelDrag(input)` resets all drag and pending-promotion state immediately (called on `WM_RBUTTONDOWN` or missed button-up recovery).

`InputInit(input)` initialises the struct to "not dragging, no pending promotion" at startup.

`DrawPromotionPicker(rs, board_x, board_y, square_size, to_rank, to_file, promoting_side)` renders the four picker squares over the board using a golden highlight (`BOARD_PROMO_PICK`) with the corresponding piece icon drawn inside each square.  `DrawBoard` is called first with `hide_rank/hide_file` pointing at the source pawn so the board appears without the moving pawn while the picker is visible.

#### Win32 Integration

All three drag messages are handled in `WindowProc`; pixel coordinates are read from `lparam` via signed 16-bit casts (`(int16)LOWORD(lparam)`) to correctly handle coordinates that extend off the client area.

`SetCapture(window)` is called immediately after a successful drag start (i.e., when `g_InputState->dragging` is `true` after `InputHandleDragStart`) so that the window continues to receive `WM_MOUSEMOVE` and `WM_LBUTTONUP` even when the cursor leaves the client area.  `ReleaseCapture()` is called on every `WM_LBUTTONUP` and `WM_RBUTTONDOWN` handler, regardless of whether a drag was active.

`WM_CAPTURECHANGED` is handled carefully: it calls `InputCancelDrag` **only when `g_InputState->dragging` is true**.  When a drag ends on the last rank, `InputHandleDragEnd` sets `pending_promotion = true` and then `WM_LBUTTONUP` calls `ReleaseCapture()`, which immediately fires `WM_CAPTURECHANGED`.  Because the drag sub-state is already cleared at that point, the guard prevents the expected post-drop capture loss from incorrectly cancelling the promotion picker.  When the OS revokes capture externally (e.g. an `Alt+Tab` or modal dialog while the button is still held down), `dragging` is still true, so `InputCancelDrag` is called normally.

The board layout constants (`BOARD_X = 320`, `BOARD_Y = 40`, `BOARD_SQUARE_SIZE = 80`) are defined as file-scope `static const int32` in `src/main.cpp` and shared between the render loop and `WindowProc`.

### User Interface

Own board presentation, drag-and-drop interaction state, move feedback, and visible status messages such as check or checkmate.

The UI is implemented in `src/ui.h` and `src/ui.cpp`.

#### Board Rendering

`DrawBoard(rs, gs, board_x, board_y, square_size, selected_rank, selected_file, legal_moves, hide_rank, hide_file)` draws the complete board view into the renderer's pixel buffer in two passes:

1. **Square pass** — draws all 64 squares with alternating light/dark colours (chess.com palette: `#F0D9B5` light, `#B58863` dark). Highlighted squares (selected piece and valid-move targets) are tinted green. Valid-move targets additionally display a small dot (empty squares) or a corner ring (occupied squares).

2. **Piece pass** — calls the internal `DrawPiece` for every non-empty square, except the square identified by `hide_rank`/`hide_file` (used during drag so the piece renders only at the cursor rather than on the board). Piece icons are drawn procedurally using `DrawRect` and `DrawFilledCircle` with shapes scaled proportionally to `square_size`. White pieces use a near-white fill with a dark outline; black pieces use a near-black fill with a light outline. Each piece type has a distinct silhouette:

| Piece | Shape |
|---|---|
| Pawn | Circle head + flat base |
| Rook | Rectangle tower with three battlements |
| Knight | Asymmetric staircase (body + offset head + snout) |
| Bishop | Ball on a narrow stem + flat base + tip orb |
| Queen | Large circle body + three crown orbs |
| King | Rectangle body + prominent cross on top |

`DrawPieceAt(rs, type, color, center_x, center_y, sq_size)` renders a single piece centered at an arbitrary pixel position.  Called by the render loop to draw the floating piece under the cursor during a drag.

`DrawPromotionPicker(rs, board_x, board_y, square_size, to_rank, to_file, promoting_side)` overlays four golden-highlighted squares at the promotion file (Queen, Rook, Bishop, Knight in slot order) so the player can click to choose a promotion piece.

`DrawGameOverOverlay(rs, result, board_x, board_y, square_size)` draws a centered panel on top of the board when the game has ended.  The panel consists of:

- A dark background rectangle with a light-gray border.
- A result strip (top 60 px of the panel) coloured near-white for a White win, dark gray for a Black win, or mid-gray for a draw, with a king-piece icon identifying the winning side (both kings shown for a draw).
- A green restart button (lower portion of the panel) with a ring icon.  Its pixel bounds are the same values returned by `IsRestartButtonHit`.

`IsRestartButtonHit(px, py, board_x, board_y, square_size)` returns `true` when `(px, py)` falls within the restart button drawn by `DrawGameOverOverlay`.  The button is computed relative to the board layout constants so it scales correctly if `square_size` changes.  With the standard layout (`board_x=320`, `board_y=40`, `square_size=80`) the button occupies pixel rows 360–419 and pixel columns 500–779.

The board is centered in the 1280×720 window at render time (`board_x = 320`, `board_y = 40`, `square_size = 80`). The window is created with `AdjustWindowRect` so that the client area is exactly 1280×720 regardless of title-bar height.

### In-Application Tests

Own deterministic validation of subsystem behavior inside the application executable. Tests should be able to exercise chess rules and other low-level logic without requiring human interaction.

## Dependency Direction

The repository should bias toward the following dependency shape:

`platform -> renderer -> ui -> game_state -> chess_rules`

Support systems such as memory and test infrastructure may be referenced where needed, but dependencies should remain one-directional and easy to explain.

## Architectural Non-Goals

- No class hierarchies for pieces or screens.
- No runtime plugin system.
- No dependency on external rendering, windowing, or test frameworks.
