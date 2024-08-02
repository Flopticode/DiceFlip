<h1>Dice Flip - what is this project?</h1>
I used the <a href="https://en.wikipedia.org/wiki/Minimax">minimax algorithm</a> to make a bot playing a two-player dice game perfectly.
Note that the code written for this project is not the yellow from the egg, so expect your pig to be whistling while reading this.
This game is not fair at all as you can see in the <a href="https://github.com/Flopticode/DiceFlip/blob/master/DiceFlip/results.txt">game evaluation table</a>.

<h2>The game evaluation table</h2>
The resulting data can be found <a href="https://github.com/Flopticode/DiceFlip/blob/master/DiceFlip/results.txt">here</a>. The following table contains explanations for every attribute of the data entries.

| Attribute Name | Description                                                                  |
| :---           |    :---                                                                      |
| dice           | The dice state at game start                                                 |
| startingplayer | The ID of the player moving first (-1 = Player, 1 = Computer)                |
| total          | The starting total                                                           |
| eval           | The game's outcome if played perfectly (-1 = Player wins, 1 = Computer wins) |
