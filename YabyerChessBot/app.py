#web based chess GUI, run command python3 app.py
from flask import Flask
from flask import request
from flask import render_template
import chess
import chess.engine
#create engine
engine = chess.engine.SimpleEngine.popen_uci('./engine/yabyerBot')
app = Flask(__name__)
#define root
@app.route('/')
def root():
    return render_template('yycb.html') #default root file

#make move API, allow for post requests from game state to reach the UCI
@app.route('/make_move', methods=['POST']) 
def make_move():
    #Initialize the starting FEN string
    fen = request.form.get('fen')
    fixed_depth = request.form.get('fixed_depth')
    move_time = request.form.get('move_time')
    #initialize board
    board = chess.Board(fen)
    #if move time has been selected...
    if move_time == '0':
            #find best move w/ instant time
        info = engine.analyse(board, chess.engine.Limit(time=0.1))
    else:
       
            #use time given
        info = engine.analyse(board, chess.engine.Limit(time = int(move_time)))
    
    if fixed_depth != '0':
       
        info = engine.analyse(board, chess.engine.Limit(depth = int(fixed_depth)))
            
    best_move = info['pv'][0]
    #update internal baord state
    board.push(best_move)
    #send board state to the FEN
    fen = board.fen()
   

    
        


    return {
      'fen': fen,
        'best_move': str(best_move),
        'score': str(info['score']),
        'depth' : info['depth'],
        'pv': ' '.join([str(move) for move in info['pv']]),
        'nodes': info['nodes'],
        'time': info['time']
    }
#main driver
#run class directly or to another module
if __name__ == '__main__':
    #start http server, restart server automatically (debug), threaded allows for  multiuser interaction
    app.run(host='localhost', port=9874,debug =True, threaded = True)
