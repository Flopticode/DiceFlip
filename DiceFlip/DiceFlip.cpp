#include <iostream>
#include <vector>
#include "types.h"
#include <string>
#include <fstream>

#define AUTOINPUT

/**
 * The output file where some results can be written to
 */
static std::ofstream file;

/**
 * The maximum number of GameStates. The hash function must guarantee 1-universality in {0, 1, ..., NUM_GAME_STATES}.
 */
static constexpr unsigned long long NUM_GAME_STATES = 4294967296;

/**
 * The datatype representing a player. -1 for player 1, 1 for player 2
 */
typedef int8 Player;

/**
 * The datatype representing a move. Values are {1, 2, ..., 6} corresponding to the number the dice is showing
 */
typedef uint8 Move;

/**
 * The datatype representing a node type used for alpha-beta-pruning.
 * 1: EXACT
 * 2: LOWER
 * 3: UPPER
 */
typedef uint8 NodeType;

/**
 * Constant representing the EXACT node type
 */
static constexpr NodeType NODE_TYPE_EXACT = 1;

/**
 * Constant representing the LOWER node type
 */
static constexpr NodeType NODE_TYPE_LOWER = 2;

/**
 * Constant representing the UPPER node type
 */
static constexpr NodeType NODE_TYPE_UPPER = 3;

/**
* Represents a complete GameState. Note that 'winner' is set to zero for optimization reasons if no winner is set even though the value is not
* specified by the Player type.
*/
typedef struct _GameState {
	Move lastMove;
	int8 total;
	Player activePlayer;
	Player winner;
} GameState;

/**
 * The transposition table used to avoid unnecessary calculations.
 * The n-th entry in the table corresponds to the GameState with the hash value n. Note that this calculation is collision-free due
 * to the hash function being bijective.
 * Values of the transposition table are bitmasks in the form
 * AAAAAAAABBCCCCCC, where
 * A is the evaluation depth of the entry
 * B is the node type and
 * C is the evaluation
 */
static uint16* transpositionTable = nullptr;

/**
* The hash function for GameState. Hash values are used to reference entries of the transposition table
*/
static inline constexpr uint32 hash(const GameState& state) noexcept
{
	uint32 out = 0;
	return out
		| (static_cast<uint8>(state.total + 128))
		| (static_cast<uint8>(state.activePlayer + 128) << 8)
		| (static_cast<uint8>(state.winner + 128) << 16)
		| (state.lastMove << 24);
}

/**
 * Creates a transposition table entry value. See the definition of `transpositionTable` for details
 */
static inline constexpr uint16 makeTTVal(const uint8& depth, const int8& eval, const NodeType& type)
{
	return (static_cast<uint16>(depth) << 8)
		| (static_cast<NodeType>(type) << 6)
		| (static_cast<uint8>(eval));
}

/**
 * Extracts the depth value from a transposition table entry
 */
static inline constexpr uint8 getDepth(uint32 hash) noexcept
{
	return transpositionTable[hash] >> 8;
}

/**
 * Extracts the evaluation value from a transposition table entry
 */
static inline constexpr int8 getRating(uint32 hash) noexcept
{
	return static_cast<int8>(transpositionTable[hash] & 0b111111);
}

/**
 * Extracts the node type from a transposition table entry
 */
static inline constexpr NodeType getNodeType(uint32 hash) noexcept
{
	return static_cast<NodeType>((transpositionTable[hash] >> 6) & 0b11);
}

/**
 * Creates a new game state where nobody is the winner yet
 */
static constexpr GameState createGameState(const Move& lastMove, const Player& player, const int8& total) noexcept
{
	return GameState{lastMove, total, player, 0};
}

/**
 * Returns a random number in {1, 2, ..., 6}
 */
static inline uint8 rollDice()
{
	return (rand() % 6) + 1;
}

/**
 * Performs the given `Move` on the given `GameState` and returns the resulting `GameState`
 */
static inline constexpr GameState performMove(const GameState& state, const Move& move) noexcept
{
	int8 newTotal = state.total - move;
	return GameState{
		move,
		static_cast<int8>(newTotal),
		static_cast<Player>(-state.activePlayer),
		static_cast<Player>((newTotal <= 0) * (-state.activePlayer))};
}

/**
 * Performs every valid move on the given `GameState` and writes the results in the `out` array
 */
static inline void getPossibleStates(GameState out[4], const GameState& lastState)
{	
	switch (lastState.lastMove)
	{
	case 6:
	case 1:
	{
		out[0] = performMove(lastState, 5);
		out[1] = performMove(lastState, 4);
		out[2] = performMove(lastState, 3);
		out[3] = performMove(lastState, 2);
		return;
	}
	case 5:
	case 2:
	{
		out[0] = performMove(lastState, 6);
		out[1] = performMove(lastState, 4);
		out[2] = performMove(lastState, 3);
		out[3] = performMove(lastState, 1);
		return;
	}
	case 4:
	case 3:
	{
		out[0] = performMove(lastState, 6);
		out[1] = performMove(lastState, 5);
		out[2] = performMove(lastState, 2);
		out[3] = performMove(lastState, 1);
		return;
	}
	}
}

/**
 * Evaluates the plain state as a base case of the minimax recursion
 */
static inline int8 eval(const GameState& state)
{
	return (state.total <= 0) * state.winner;
}

/**
 * Performs the minimax algorithm on the `curState` with a maximum depth of `depth`. `alpha` and `beta` values are used for
 * alpha-beta-pruning. The first call should pass the minimum evaluation value for `alpha` and the maximum evaluation value for `beta`
 */
static int8 miniMax(const GameState& curState, const uint8& depth, int8 alpha, int8 beta)
{
	// 
	// Check if base case applies
	// 
	if (depth == 0 || curState.total <= 0)
		return curState.activePlayer * eval(curState);

	int8 max = -1;

	const Player player = curState.activePlayer;

	GameState gameStates[4];
	getPossibleStates(gameStates, curState);

	// 
	// Check possible next states
	// 

	for(uint8 i = 0; i < 4; i++)
	{
		GameState possibleNext = gameStates[i];

		// 
		// Transposition table lookup
		// 

		uint32 hashVal = hash(possibleNext);
		uint8 ttDepth = getDepth(hashVal);
		if (ttDepth > 0)
		{
			if (ttDepth >= depth)
			{
				int8 eval = getRating(hashVal);

				switch (getNodeType(hashVal))
				{
				case NODE_TYPE_LOWER:
				{
					if (eval > alpha)
						alpha = eval;
				} break;
				case NODE_TYPE_UPPER:
				{
					if (eval < beta)
						beta = eval;
				} break;
				case NODE_TYPE_EXACT:
				{
					return eval;
				}
				}

				if (alpha >= beta)
				{
					return eval > max ? eval : max;
				}
			}
		}

		// 
		// Perform minimax
		// 

		int8 val = -miniMax(possibleNext, depth - 1, alpha, beta);
		if (val > max)
		{
			max = val;
		}

		// 
		// Update transposition table
		// 

		if(val <= alpha)
			transpositionTable[hashVal] = makeTTVal(depth, val, NODE_TYPE_UPPER);
		else if (val >= beta)
			transpositionTable[hashVal] = makeTTVal(depth, val, NODE_TYPE_LOWER);
		else
		{
			uint8 uVal = static_cast<uint8>(val + 128);

			transpositionTable[hashVal] = makeTTVal(depth, val, NODE_TYPE_EXACT);
		}
	}

	return max;
}

/**
 * Chooses the best move and performs it.
 */
GameState makeBestMove(const GameState& curState)
{
	static GameState nextPossible[4];
	getPossibleStates(nextPossible, curState);

	int8 max = -2;
	uint8 maxI = 0xFF;

	for (uint8 i = 0; i < 4; i++)
	{
		int8 evaluation = -miniMax(nextPossible[i], 100, -128, 127);

		if (evaluation >= max)
		{
			max = evaluation;
			maxI = i;
		}
	}

	if (max == -2)
		return nextPossible[0];

	return nextPossible[maxI];
}

/**
 * The main function. Iterates over every possible game and plays it. Depending on the `AUTOINPUT` definition above, the computer
 * can play with itself.
 */
int main()
{
	file.open("./results.txt", std::ios::out);

	// 
	// Transposition table initialization
	// 

	transpositionTable = reinterpret_cast<uint16*>(calloc(sizeof(uint16), NUM_GAME_STATES));

	// 
	// Looping over every game possible
	// 

	for (int8 startTotal = 66; startTotal >= 11; startTotal--)
	{
		for (Move startMove = 1; startMove <= 6; startMove++)
		{
			for (Player startPlayer = -1; startPlayer <= -1; startPlayer += 2)
			{
				// 
				// Game Initialization
				//

				std::cout << "Starting total: " << std::to_string(startTotal) << std::endl;
				std::cout << "Dice shows: " << std::to_string(startMove) << std::endl;

				GameState curState = createGameState(startMove, startPlayer, startTotal);

				// 
				// Computer flexes its abilites
				// 

				int8 eval = miniMax(curState, 100, -128, 127);
				if (startPlayer == -1)
					eval = -eval;

				std::cout << "[" << std::to_string(eval) << "] The computer already knows " << (eval > 0 ? "it" : (eval < 0 ? "you" : "nobody")) << " will win if played perfectly." << std::endl;
		
				std::string str = "{dice:" + std::to_string(curState.lastMove) + ",startingplayer:" + std::to_string(curState.activePlayer) + ",total:" + std::to_string(curState.total) + ",eval:" + std::to_string(eval) + "}\n";
				file << str;

				if (startPlayer == 1)
					std::cout << "Computer starts." << std::endl;
				else
					std::cout << "You start." << std::endl;

				// 
				// Game starts
				// 

				while (curState.winner == 0)
				{
					if (curState.activePlayer == 1)
					{
						// 
						// Computer's turn
						// 

						curState = makeBestMove(curState);

						std::cout << "[" << std::to_string(miniMax(curState, 63, -128, 127)) << "] The computer turns to " << std::to_string(curState.lastMove) << ".New total is " << std::to_string(curState.total) << std::endl;

					}
					else
					{
						// 
						// Player's turn
						// 

						Move move = '?';

#ifndef AUTOINPUT
						std::cout << "Your move: ";
						std::cin >> move;
#endif // AUTOINPUT

						if (move == '?')
						{
							// 
							// Automatically make best move if player enters '?' or autoinput is active
							// 

							GameState bestState = makeBestMove(curState);

							std::cout << "(You move " << std::to_string(bestState.lastMove) << ")" << std::endl;
							move = bestState.lastMove;
						}
						else
						{
							// 
							// Translate ASCII to move
							// 

							move -= '0';
						}

						// 
						// Check move validity
						// 

						if (move + curState.lastMove == 7 || move == curState.lastMove)
						{
							std::cout << "## Your move is invalid ##" << std::endl;
							continue;
						}

						// 
						// Perform the move
						// 

						curState = performMove(curState, move);
					}
				}

				if (curState.winner == 1)
				{
					std::cout << "Computer wins." << std::endl;
					if (eval < 1)
						
					{
						std::cout << "huh?";
						return 0;
					}
				}
				else if (curState.winner == -1)
				{
					std::cout << "You win." << std::endl;

					if (eval > -1)
					{
						std::cout << "huh?";
						return 0;
					}
				}

				//std::cout << "Minimax evaluates " << std::to_string(evaluation) << std::endl;
			}
		}
	}

	file.close();
	
}
