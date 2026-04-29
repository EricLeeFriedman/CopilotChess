# Requirements

## Product

- The application is a chess game.
- All pieces and moves must obey standard chess rules.
- The game must show check and checkmate to the player.
- After a winner is decided, the game must allow a restart.

### Chess Rule Invariants

These invariants are authoritative for both implementation and review. When in doubt, defer to FIDE Laws of Chess.

- **Attacked-square semantics**: A pinned enemy piece still attacks squares it cannot legally move to. Attack detection for king-safety purposes (king moves, castling, `IsInCheck`) uses pseudo-legal piece reachability, not filtered legal moves. (FIDE Article 3.8)
- **Legal move filter**: No legal move may result in the moving side remaining in check. The legal-move pass applies after generating candidate moves.
- **King-capture exclusion**: A move that would capture the opposing king is not a legal move. Legal move generators must filter these out.
- **Castling legality**: The king may not start on, pass through, or land on a square attacked by the opponent. Attack checks for transit and destination squares must use a board with the king removed from its starting square, so the king does not shield sliding-piece rays targeting those squares.
- **Pawn attack direction**: A pawn attacks the two diagonal squares in its direction of advance only. A pawn push forward does not attack or threaten squares.
- **Promotion**: When a pawn reaches the opponent's back rank, it must be promoted to queen, rook, bishop, or knight (player's choice). All four piece types must be selectable. Promotion applies identically for both White and Black.

## Platform And UX

- Windows only.
- 2D software rendering through the Windows API.
- Mouse-only local play for two players on one machine.
- Piece movement is click-and-drag:
  - Left-button down on a piece belonging to the current player picks it up.
  - Moving the mouse while holding the button drags the piece; legal-move targets are highlighted on the board.
  - Left-button up drops the piece. If the drop square is a legal target the move is applied; otherwise the piece returns to its origin.
  - Right-button cancels an in-progress drag and returns the piece to its origin.
  - Mouse capture is set on drag start and released on drop or cancel so that button-up is received even when the cursor leaves the client area.
- Pawn promotion:
  - When a pawn is dropped on the last rank a promotion picker overlay appears at the promotion file showing Queen, Rook, Bishop, and Knight (in that order away from the promotion rank).
  - Clicking one of the four picker squares applies the promotion with the chosen piece.
  - Clicking outside the picker, or right-clicking, cancels the promotion and leaves the board unchanged.
- Game restart:
  - After checkmate or stalemate the game-over overlay is displayed over the board showing a result indicator and a green restart button.
  - Restarting is triggered exclusively by **clicking the restart button** with the left mouse button — no keyboard shortcut exists.
  - Clicking anywhere other than the restart button while the game-over overlay is visible has no effect.
  - Restart resets the board to the standard starting position, sets White to move, clears the en-passant target, restores all castling rights, and clears any active drag or pending-promotion state.

## Build And Source Control

- The build entry point will be a simple `build.ps1` script that invokes `cl.exe`.
- The repository uses GitHub for source control and collaboration.
- GitHub-native workflow assets should be preferred when they help agents work consistently.

## Code Style And Architecture

- C++ implementation.
- No object-oriented programming as a primary structuring mechanism.
- Use C-style APIs at module boundaries and plain structs with public data.
- Avoid dynamic memory allocation during runtime.
- Reserve memory up front and divide it into separate arenas for different systems.
- Do not use third-party libraries.
- Avoid the C standard library as much as practical.

## Testing

- Testing lives inside the application.
- The executable must support a dedicated testing mode.
- The test suite is owned and maintained as part of the product, not as a separate harness application.

## Repository Bootstrapping Constraint

The initial repository commit is intentionally limited to workflow scaffolding and documentation. No application code is introduced in this stage.
