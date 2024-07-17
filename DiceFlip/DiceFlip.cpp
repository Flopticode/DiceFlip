#include <iostream>
#include <vector>
#include "types.h"

typedef uint8 Move;

/* Bitmask: AAABBBBBBB
*  A: Dice top number
*  B: Remaining total
*/
typedef uint16 GameState;

static constexpr Move MOVE_ONE = 0b1;
static constexpr Move MOVE_TWO = 0b10;
static constexpr Move MOVE_THREE = 0b100;
static constexpr Move MOVE_FOUR = 0b1000;
static constexpr Move MOVE_FIVE = 0b10000;
static constexpr Move MOVE_SIX = 0b100000;

static inline GameState performMove(GameState state, Move move)
{
	uint8 totalBefore = (state << 3);
	return move | (totalBefore >> 3);
}

static constexpr Move moves[] = {
	MOVE_ONE, MOVE_TWO, MOVE_THREE, MOVE_FOUR, MOVE_FIVE, MOVE_SIX
};

static inline uint8 rollDice()
{
	return (rand() % 6) + 1;
}
static inline Move randomMove()
{
	return (rand() % 6);
}

static constexpr uint8 getValidMoves(Move lastMove)
{
	switch (lastMove)
	{
	case MOVE_SIX:
	case MOVE_ONE:
		return MOVE_TWO | MOVE_THREE | MOVE_FOUR | MOVE_FIVE;
	case MOVE_TWO:
	case MOVE_FIVE:
		return MOVE_ONE | MOVE_THREE | MOVE_FOUR | MOVE_SIX;
	case MOVE_THREE:
	case MOVE_FOUR:
		return MOVE_ONE | MOVE_TWO | MOVE_FIVE | MOVE_SIX;
	}
	return 0;
}

int main()
{
	srand(0);

	uint8 startNum = rollDice() * 10 + rollDice();
	Move startMove = moves[rand() % 6];


}
