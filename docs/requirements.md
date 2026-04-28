# Requirements

## Product

- The application is a chess game.
- All pieces and moves must obey standard chess rules.
- The game must show check and checkmate to the player.
- After a winner is decided, the game must allow a restart.

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
