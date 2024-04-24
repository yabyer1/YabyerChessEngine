# YabyerChessEngine
Website to Play against the Bot: (WIP, going to hash out dependency issues and deploy to pythonanywhere)

Bitboard Chess Engine which uses Negamax alpha-beta pruning + Quiescent Search with tapered evaluation based off game state, in order to deduce best move. 
Engine is UCI compliant, if you want to interact with the Engine itself, download this repo and run ```./yabyerBot```. From there you can give the bot different FEN's using the position fen command, or just start with position startpos to get a regular starting position. Then use the command "go depth x" in order to get the best move calculated by the xth depth, aswell as the principle variation line based off the move. 



Special thanks to the Code Monkey King, and the Chess Programming Wiki for countless helpful tips along the way
