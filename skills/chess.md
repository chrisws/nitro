# Chess

Use this to play or coach a game of chess against the user: render the board in Unicode after
every move, and collect the user's move via `TOOL:ASK`. The single biggest failure mode in an
LLM playing chess is silently losing track of the actual board state — treat state-keeping as
the core discipline of this skill, not an afterthought to move generation.

## Board state: keep an explicit source of truth

- Maintain the position as a FEN string (or an 8x8 array you can print) in your own working
  notes, updated after every single move — yours and the user's. Never reconstruct the board
  from memory of "what's probably still there" a few moves later; always update the explicit
  state immediately when a move is made, then render *from that state*, not from recollection.
- Before rendering, sanity-check the state: exactly one king per side, no pawns on rank 1/8,
  piece counts that make sense given captures so far. If something looks off, stop and recheck
  the last few moves rather than rendering an inconsistent board.
- Track alongside the board: whose turn it is, castling rights remaining (each side,
  kingside/queenside, invalidated the moment the king or that rook moves), en passant
  availability (only legal the immediate move after a double pawn push), and halfmove clock if
  you're tracking draw conditions.

## Rendering the board

Use Unicode chess glyphs, one per square, in an 8x8 grid with rank/file labels. White pieces:
♔♕♖♗♘♙. Black pieces: ♚♛♜♝♞♟. Example starting position, White at the bottom:

```
  a b c d e f g h
8 ♜ ♞ ♝ ♛ ♚ ♝ ♞ ♜ 8
7 ♟ ♟ ♟ ♟ ♟ ♟ ♟ ♟ 7
6 · · · · · · · · 6
5 · · · · · · · · 5
4 · · · · · · · · 4
3 · · · · · · · · 3
2 ♙ ♙ ♙ ♙ ♙ ♙ ♙ ♙ 2
1 ♖ ♘ ♗ ♕ ♔ ♗ ♘ ♖ 1
  a b c d e f g h
```

- Keep the same orientation for the whole game (ask once at the start which color the user
  wants, and orient the board with their side at the bottom) — flipping orientation mid-game is
  disorienting and a common source of the user misreading the board.
- Use `·` (middle dot) for empty squares, not blank space — it keeps the grid visually aligned
  and makes miscounted rows obvious at a glance.
- Re-render the full board after every move, even a "small" one — don't describe a move in
  prose only and skip the redraw; the board is the shared source of truth between you and the
  user, not your narration of it.

## Collecting and validating moves

- Use `TOOL:ASK` to get the user's move each turn. Accept standard algebraic notation (`Nf3`,
  `exd5`, `O-O`, `Qxh7#`) and also plain from-to squares (`e2e4`) — don't force one format if
  the intent is unambiguous.
- Before applying a user's move, check it's actually legal in the current position: the piece
  exists on the stated square (or the only piece that could make the stated algebraic move),
  the path is unobstructed for sliding pieces, the destination doesn't leave their own king in
  check, and any special-move preconditions hold (castling rights intact and squares unattacked
  for castling; correct rank and immediately-prior double push for en passant).
- If a move is illegal or ambiguous, say specifically why (not just "invalid move") and re-ask
  via `TOOL:ASK` rather than guessing at what they meant.
- Apply your own moves with the same legality discipline — verify the move is legal in the
  tracked position before committing to it and updating state, not just plausible-looking.

## Playing vs. coaching mode

- **Playing mode**: pick a reasonable move for the position — prioritize not hanging material,
  taking free material when offered, and basic king safety over deep calculation. State the
  move in algebraic notation alongside the redraw; a short reason is fine but keep it brief so
  the game keeps moving.
- **Training/coaching mode**: after the user's move, before playing your own, give brief
  feedback — was it sound, did it hang something, was there a stronger option — then continue.
  Calibrate the depth of feedback to what was asked: a beginner wants "you left your knight
  undefended," not a six-ply engine line.
- Ask up front (or infer from how the request was phrased) whether the user wants straight play
  or commentary along the way — the two modes read very differently turn to turn.

## Ending the game

- Check for checkmate, stalemate, threefold repetition (if you're tracking move history),
  insufficient material, and the 50-move rule as they come up, and call the result plainly when
  one is reached rather than continuing to play out a finished position.
- On check (not mate), say so explicitly in the move announcement — don't rely on the board
  alone to communicate it.

## What NOT to do

- Don't narrate a move without re-rendering the board — the render is the check against your
  own state drifting from reality.
- Don't apply an illegal move because it "looks plausible" — verify against the tracked state,
  every time, for both sides.
- Don't silently forget captured pieces, en passant rights, or castling eligibility — these are
  exactly the state that's easy to lose track of over a long game and easy to verify if you keep
  explicit notes.
- Don't switch board orientation mid-game.
