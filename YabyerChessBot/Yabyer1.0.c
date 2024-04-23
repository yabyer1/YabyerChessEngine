//1.0 version 1700 ELO


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define U64 unsigned long long
/*

Time functions

*/
//Get time 

//exit from engine
int quit = 0;
//UCI  commands
int movestogo = 30;
int movetime = -1;
int times = -1;
int inc = 0;
int starttime = 0;
int stoptime = 0;
int timeset = 0;
int stopped = 0;
int get_time_ms(){
    struct timeval time_value;
    gettimeofday(&time_value , NULL);
    return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
}
//listen to User input from the STDIN
int input_waiting(){
   fd_set readfds;
   struct timeval tv;
   FD_ZERO(&readfds);
   FD_SET(fileno(stdin), &readfds);
   tv.tv_sec=0;tv.tv_usec = 0;
   select(16, &readfds, 0, 0, &tv);
   return (FD_ISSET(fileno(stdin), &readfds));
}// read GUI/user input
void read_input()
{
    // bytes to read holder
    int bytes;
    
    // GUI/user input
    char input[256] = "", *endc;

    // "listen" to STDIN
    if (input_waiting())
    {
        // tell engine to stop calculating
        stopped = 1;
        
        // loop to read bytes from STDIN
        do
        {
            // read bytes from STDIN
            bytes=read(fileno(stdin), input, 256);
        }
        
        // until bytes available
        while (bytes < 0);
        
        // searches for the first occurrence of '\n'
        endc = strchr(input,'\n');
        
        // if found new line set value at pointer to 0
        if (endc) *endc=0;
        
        // if input is available
        if (strlen(input) > 0)
        {
            // match UCI "quit" command
            if (!strncmp(input, "quit", 4))
            {
                // tell engine to terminate exacution    
                quit = 1;
            }

            // // match UCI "stop" command
            else if (!strncmp(input, "stop", 4))    {
                // tell engine to terminate exacution
                quit = 1;
            }
        }   
    }
}
// a bridge function to interact between search and GUI input
static void communicate() {
	// if time is up break here
    if(timeset == 1 && get_time_ms() > stoptime) {
		// tell engine to stop calculating
		stopped = 1;
	}
	
    // read GUI input
	read_input();
}


//FEN Debug Positions
#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

//RANDOM NUMBERS
// xor shift -> random generator needed for the magic number hashing for the occupancy bits
//pspeudo randoms tate
unsigned int random_state = 1804289383;
//generate 32 bit pseudo legal numbers
unsigned int get_random_U32_number(){
    //get current state
    unsigned int number = random_state;
    //xor shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;
    //update rnadom number state
    random_state = number;
    // return number
    return number;
}
//generate 64 bit pseudo legal random number
U64 get_random_U64_number(){
    //define 4 random numbers
    U64 n1, n2, n3, n4;
    //initialize (Also slice off the 16 bits from the most significant bit (lowest bit in the bitboard representation))
    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;
    //return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}
U64 generate_candidate_magic_number(){
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}
//bit macros
//set/get/pop macros
#define set_bit(bitboard, square) (( bitboard |= (1ULL << square)))
#define get_bit(bitboard, square) ((bitboard & (1ULL << square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))
//Bit Manipulations
//Least Significant Bit Deletion
static inline int count_bits(U64 board){
    int count = 0;
    //Find amount of bits set by repeatedly deleting the lest significant bit
    while(board){
        board &= (board - 1);
        count++;
    }

    return count;
}
static inline int least_significant_bit_index(U64 board){
    //make sure not 0
    
    int count = 0;
   
    if(board){
        //count trailing bits before least significant 
       return count_bits((board & -board) - 1);
    }
else
    return -1;
}
//#define U64 not_h_file = 
//board notations
enum{
 a8, b8, c8, d8, e8, f8, g8, h8, 
a7, b7, c7, d7, e7, f7, g7, h7, 
a6, b6, c6, d6, e6, f6, g6, h6, 
a5, b5, c5, d5, e5, f5, g5, h5, 
a4, b4, c4, d4, e4, f4, g4, h4, 
a3, b3, c3, d3, e3, f3, g3, h3, 
a2, b2, c2, d2, e2, f2, g2, h2, 
a1, b1, c1, d1, e1, f1, g1, h1, No_sq
};

enum{
    rook, bishop
};
//Define  piece Bitboards
U64 bitboards[12];// 6 for each side (black and white unique peices)
const int BITBOARD_SIZE = 96;
const int OCCUPANCY_SIZE = 24;
//encode peices
enum{P, N, B, R, Q, K,  p, n , b , r , q, k};
//define occupancy bitboards, (both, black and white)
U64 occupancies[3];
enum {white, black, both};// get each an idnex

//side to move
int side = white;
 //en passant
 int enpassant = No_sq;
 //copy board state
#define copy_board()                            \
 U64 bitboards_copy[12], occupancy_copy[3];     \
  int side_copy, enpassant_copy, castle_copy;    \
  memcpy(bitboards_copy, bitboards, BITBOARD_SIZE); \
  memcpy(occupancy_copy, occupancies, OCCUPANCY_SIZE);\
  side_copy = side; enpassant_copy = enpassant; castle_copy = castle; \
  U64 hash_key_temp = hash_key; \
// restore baord state
#define restore_board()         \
  memcpy(bitboards, bitboards_copy, BITBOARD_SIZE); \
  memcpy(occupancies, occupancy_copy, OCCUPANCY_SIZE);\
  side = side_copy; enpassant = enpassant_copy; castle = castle_copy; \
  hash_key = hash_key_temp; \

enum {all_moves, only_captues};
U64 repetition_table[1000]; //Number of plies in an entire game
int repetition_index;
int ply; // half move counter



/*
Castling rights
King & Rooks didnt move: 1111 & 1111 = 1111 =  15

White king Moved -> 1111 & 1100 = 1100 = 12
White king rook moved -> 1111 & 1110 = 1110 = 14
White queen rook moved -> 1111 & 1101 = 1101 = 14


Black king Moved -> 1111 & 0011 = 0011 = 3
Black king rook moved -> 1111 & 1011 = 1011 = 11
Black queen rook moved -> 1111 & 0111 = 111 = 7


*/
// castling rights update constants
const int castling_rights[64] = {
    7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14

};

//castling rights (rep 0000 -> 1111) -> 1 (white king can castle king side), 2 (white king can castle queen side), 4 (black king can castle king side), 8 (black king can castle queen side)
//1111 -> both can castle either direction, 1001 -> black king queen side, white king king side etc
int castle;
enum {wk = 1, wq = 2, bk = 4, bq = 8};
//string board notis 
const char * s_to_c[] ={
"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", 
"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"};

//ASCII pieces
char ascii_pieces[12] = "PNBRQKpnbrqk";
//unicode pieces
char *unicode_pieces[12] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};
// convert ascii character to encoded constants
int char_pieces[] = {
['P'] = P,
['N'] = N,
['B'] = B,
['R'] = R,
['Q'] = Q,
['K'] = K,
['p'] = p,
['n'] = n,
['b'] = b,
['r'] = r,
['q'] = q,
['k'] = k,
};

//Coming over all sorts of occupancy variations on a piece on a certain attack (using  the index (combos) and attack mask)
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask){
    // occupancy map
    U64 occupancy = 0ULL;
    //loop over the range of bits in the attack mask
    for(int count = 0; count < bits_in_mask; count++){
        //get least significant bit index of attack mask
        int square = least_significant_bit_index(attack_mask);
        //Pop the bit out
        pop_bit(attack_mask, square);
        //add occupancy on board
        if(index & (1 << count))
        //populate occupancy map
        occupancy |= (1ULL << square);

    }
    //return map
    return occupancy;
}
// We need to hash our pieces, castling rights, enpassant etc. (Zobrist Hash)
//random piece keys[piece code][square]
U64 piece_keys[12][64];
// random enpassant keys
U64 enpassant_keys[64];
//random castling keys  max is 15 1111
U64 castle_key[15];
//random side key
int side_key;
//almost uniqe position identifier (we will key these positions)
U64 hash_key;
void init_rand_keys(){
    //random number state
    random_state = 1804289383;
    //initialize piece keys
    for(int piece = P; piece <= k; piece++){
        for(int square = 0; square < 64; square++){
                //init random piece keys
                piece_keys[piece][square] =  get_random_U64_number();
              //  printf("%llx\n",  piece_keys[piece][square] );
        }
    }
     for(int square = 0; square < 64; square++){
                //init random piece keys
                enpassant_keys[square] =  get_random_U64_number();
        }
        side_key = get_random_U64_number();

         for(int square = 0; square < 16; square++){
                //init random piece keys
                castle_key[square] =  get_random_U64_number();
               // printf("%llx\n", castle_key[square]);
        }
}
//generate the almost unique position identifer
U64 generate_hash_key(){
    // final hash key
    U64 final_key = 0ULL;
    //temp piece copy bitboard
    U64 bitboard;
    for(int piece = P; piece <= k; piece++){
        bitboard = bitboards[piece];
        //loop over pieces and populate 
        while(bitboard){
            //init square
            int square = least_significant_bit_index(bitboard);
            //hash piece
            final_key ^= piece_keys[piece][square];
           // printf("square: %s\n", s_to_c[square]);
            pop_bit(bitboard, square);
        }
    }
    if(enpassant != No_sq){
        final_key ^= enpassant_keys[enpassant];
    }
    final_key ^= castle_key[castle];
    if(side == black)
    final_key ^= side_key;
    //return generated hash key
    return final_key;
}
void print_bitboard(U64 bitboard){
    //loop over board ranks
    printf("\n");
    for(int rank = 0; rank < 8; rank++){
        for(int file = 0; file < 8; file++){
            //init square
            int square = rank * 8 + file;
            // print tanks
            if(!file)
            printf(" %d ", 8 - rank);
            //print bit state (1 or 0)
            printf(" %d ", get_bit(bitboard, square) ? 1 : 0);
        }
        printf("\n");
    }
    printf("\n   a  b  c  d  e  f  g  h \n\n\n");
    //print as unsinged decimal
    printf("   Bitboard %llud \n\n", bitboard);

}
void print_board(){
    // loop over board ranks
    for(int rank = 0; rank < 8; rank++){
        for(int file = 0; file < 8; file++){
            int square = rank * 8 + file;
            //define piece variable
            int piece = -1;
            //print ranks
            if(file == 0){
                printf("%d ", 8 - rank);
            }
            // loop over all bitboards, and see if there is a bit turned on there, if so we want to encode a pirce there :)
            for(int bb_piece = P; bb_piece <= k; bb_piece++){
                    if(get_bit(bitboards[bb_piece], square)){
                        piece = bb_piece;
                    }
            }
            printf(" %s", (piece == -1) ? "." : unicode_pieces[piece]);
        }
        printf("\n");
    }
    //print files
    printf("\n   a b c d e f g h \n");
    // print side to move
    printf("Side:  %s\n", (!side) ? "white" : "black");
    // print en passant square
    printf("Enpassant: %s\n", (enpassant != No_sq) ? s_to_c[enpassant] : " no" );
    //print castling rights
    printf("Castling: %c%c%c%c\n", (castle & wk) ?  'K' : '-',
                                    (castle & wq) ?  'Q' : '-',
                                    (castle & bk) ?  'k' : '-',
                                    (castle & bq) ?  'q' : '-'
    );
    //print hashkey
    printf("Hash Key -> %llx \n", hash_key);
}
// rook magic numbers
U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};
// bishop magic numbers
U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};



// relevant occupancy bits count for every square on board
const int bishop_relevant_bits[64] = {
6, 5, 5, 5, 5, 5, 5, 6, 
5, 5, 5, 5, 5, 5, 5, 5, 
5, 5, 7, 7, 7, 7, 5, 5, 
5, 5, 7, 9, 9, 7, 5, 5, 
5, 5, 7, 9, 9, 7, 5, 5, 
5, 5, 7, 7, 7, 7, 5, 5, 
5, 5, 5, 5, 5, 5, 5, 5, 
6, 5, 5, 5, 5, 5, 5, 6

};
const int rook_relevant_bits[64] = {
12, 11, 11, 11, 11, 11, 11, 12, 
11, 10, 10, 10, 10, 10, 10, 11, 
11, 10, 10, 10, 10, 10, 10, 11, 
11, 10, 10, 10, 10, 10, 10, 11, 
11, 10, 10, 10, 10, 10, 10, 11, 
11, 10, 10, 10, 10, 10, 10, 11, 
11, 10, 10, 10, 10, 10, 10, 11, 
12, 11, 11, 11, 11, 11, 11, 12 
};
/*
  Attack Tables (memoize the time needed to lookup positional play) side == 2 and squares == 64
*/
/*
 8  0  1  1  1  1  1  1  1 
 7  0  1  1  1  1  1  1  1 
 6  0  1  1  1  1  1  1  1 
 5  0  1  1  1  1  1  1  1 
 4  0  1  1  1  1  1  1  1 
 3  0  1  1  1  1  1  1  1 
 2  0  1  1  1  1  1  1  1 
 1  0  1  1  1  1  1  1  1 

   a  b  c  d  e  f  g  h 


*/
//not A file constant
/*
 8  1  1  1  1  1  1  1  0 
 7  1  1  1  1  1  1  1  0 
 6  1  1  1  1  1  1  1  0 
 5  1  1  1  1  1  1  1  0 
 4  1  1  1  1  1  1  1  0 
 3  1  1  1  1  1  1  1  0 
 2  1  1  1  1  1  1  1  0 
 1  1  1  1  1  1  1  1  0 

   a  b  c  d  e  f  g  h 


*/
//Parse FEN String
void parse_fen(char * fen){
    // reset board position and state variable
    //reset board positions
    memset(bitboards, 0ULL, sizeof(bitboards));
    //reset occupancies (bitboards)
    memset(occupancies, 0ULL, sizeof(occupancies));
    //reset side to move
    side = 0;
    enpassant = No_sq;
    castle = 0;
    //reset hash keys
    hash_key = 0ULL;
    repetition_index = 0;
    //reset repeition table
    memset(repetition_table, 0ULL, sizeof(repetition_table));
    //reset ply
    ply = 0;
    for(int rank = 0; rank < 8; rank++){

        for(int file = 0; file < 8; file++){
            
            int square = rank * 8 + file;
            // match ascii chars w/ fen
            if((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z') ){
                //init piece type
                int piece = char_pieces[*fen];
                //set piece on the bitboard
                set_bit(bitboards[piece], square);
                // increment pointer
                fen++;
            }
            // match empty squares within FEN
            if(*fen >= '0' && *fen <= '9'){
                //init offset , then take that many periods and go to next part of the fen
                int offset = *fen - '0';
                int piece = -1;
                for(int bb_piece = P; bb_piece <= k; bb_piece++){
                    if(get_bit(bitboards[bb_piece], square)){
                        piece = bb_piece;
                    }
                }
                // if no piece on current square
                if(piece == -1){
                    file--;
                }
                file += offset;
                fen++;
            }
            if(*fen == '/'){
                fen++;
            }
            }
          
        }
     // parse side to move
     *fen++;
     side = (*fen == 'w') ? white : black;
     // parse castling rights
     fen += 2;
     while(*fen != ' '){
        switch(*fen){
            case 'K' : castle |= wk; break;
            case 'Q' : castle |= wq; break;
             case 'k' : castle |= bk; break;
            case 'q' : castle |= bq; break;
            case '-': break;
        }

        *fen++;
     }
     // parse enpassant
     *fen++;
     //printf("fen: '%s' \n", fen);
     if (*fen != '-'){
            //parse enpassant file && rank
            int file = fen[0] - 'a';
            int rank = 8 - (fen[1] - '0');
            //init enpassant square
            enpassant = rank * 8 + file;
           // printf("%d  %d \n", file, rank);
     }
     //no enpassant
     else enpassant = No_sq;

     //init white occupancies
     for(int piece = P; piece <= K; piece++){
        //populate white occupancy bitboard
        occupancies[white] |= bitboards[piece];
     }
     //BLACK
      for(int piece = p; piece <= k; piece++){
        //populate white occupancy bitboard
        occupancies[black] |= bitboards[piece];
     }
     //Do Both
     occupancies[both] |= (occupancies[white] | occupancies[black]);
    //init hash key
    hash_key = generate_hash_key();
}

/*


Attacks

*/
//not a file constant
const  U64 not_a_file = 18374403900871474942ULL;
// not H file constant
const U64 not_h_file = 9187201950435737471ULL;
// not HG file constant
const U64 not_hg_file = 4557430888798830399ULL; 
//not AB file 
const U64 not_ab_file = 18229723555195321596ULL;
//Pawn Attacks table 
U64 pawn_attacks[2][64];
//Knight Attack Table
U64 knight_attacks[64];
//King Attack Table
U64 king_attacks[64];
//Bishop attack masks
U64 bishop_masks[64];
//Rook attack masks
U64 rook_masks[64];
//bishop attacks tables [square][occupancies]
U64 bishop_attacks[64][512];
//Rook attacks tables [square][occupancies]
U64 rook_attacks[64][4096];
//On Fly attacks (hit edges)
U64 rook_attacks_on_fly(int square, U64 block){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
    // ranks and files 
    int r, f; 
    int tr = square / 8; 
    int tf = square % 8;
    // mask buits
     //each direction diagonally
     for( f = tf + 1;  f <= 7; f++){
        attacks |= (1ULL << (tr * 8 + f));
          if((1ULL << (tr * 8 + f)) & block){
            break;
        }
     }
     for(f = tf - 1;f >= 0; f--){
        attacks |= (1ULL << (tr * 8 + f));
          if((1ULL << (tr * 8 + f)) & block){
            break;
        }
     }
      for(int r = tr -1; r >= 0; r--){
        attacks |= (1ULL << (r * 8 + tf));
          if((1ULL << (r * 8 + tf)) & block){
            break;
        }
     }
       for(int r = tr + 1; r <= 7; r++){
        attacks |= (1ULL << (r * 8 + tf));
          if((1ULL << (r * 8 + tf)) & block){
            break;
        }
     }
    return attacks;
}
U64 bishop_attacks_on_fly(int square, U64 block){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
    // ranks and files 
    int r, f; 
    int tr = square / 8; 
    int tf = square % 8;
    // mask buits
     //each direction diagonally
     for(int r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++){
        attacks |= (1ULL << (r * 8 + f));
        if((1ULL << (r * 8 + f)) & block){
            break;
        }
     }
     for(int r = tr -1, f = tf - 1; r >= 0 && f >= 0; r--, f--){
        attacks |= (1ULL << (r * 8 + f));
         if((1ULL << (r * 8 + f)) & block){
            break;
        }
     }
      for(int r = tr -1, f = tf + 1; r >= 0 && f <= 7; r--, f++){
        attacks |= (1ULL << (r * 8 + f));
         if((1ULL << (r * 8 + f)) & block){
            break;
        }
     }
       for(int r = tr + 1, f = tf - 1;   r < 8 && f >= 0; r++, f--){
        attacks |= (1ULL << (r * 8 + f));
         if((1ULL << (r * 8 + f)) & block){
            break;
        }
     }
    return attacks;
}
U64 mask_king_attacks(int square){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
     // -> white
        if ( not_a_file & (bitboard >> 7)) attacks  |= (bitboard >> 7); 
        if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9); 
        if ( not_h_file & (bitboard >> 1)) attacks  |= (bitboard >> 1); 
       if(bitboard >> 8) attacks |= (bitboard >> 8);
        // blavk
       if(bitboard << 8) attacks |= (bitboard << 8);
            if ( not_h_file & (bitboard << 7)) attacks  |= (bitboard << 7); 
        if((bitboard << 9) & not_a_file) attacks |= (bitboard << 9); 
         if ( not_a_file & (bitboard << 1)) attacks  |= (bitboard << 1);
    return attacks;
}
U64 mask_rook_attacks(int square){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
    // ranks and files 
    int r, f; 
    int tr = square / 8; 
    int tf = square % 8;
    // mask buits
     //each direction diagonally
     for( f = tf + 1;  f <= 6; f++){
        attacks |= (1ULL << (tr * 8 + f));
     }
     for(f = tf - 1;f >= 1; f--){
        attacks |= (1ULL << (tr * 8 + f));
     }
      for(int r = tr -1; r >= 1; r--){
        attacks |= (1ULL << (r * 8 + tf));
     }
       for(int r = tr + 1; r <= 6 ; r++){
        attacks |= (1ULL << (r * 8 + tf));
     }
    return attacks;
}
U64 mask_bishop_attacks(int square){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
    // ranks and files 
    int r, f; 
    int tr = square / 8; 
    int tf = square % 8;
    // mask buits
     //each direction diagonally
     for(int r = tr + 1, f = tf + 1; r<= 6 && f <= 6; r++, f++){
        attacks |= (1ULL << (r * 8 + f));
     }
     for(int r = tr -1, f = tf - 1; r >= 1 && f >= 1; r--, f--){
        attacks |= (1ULL << (r * 8 + f));
     }
      for(int r = tr -1, f = tf + 1; r >= 1 && f <= 6; r--, f++){
        attacks |= (1ULL << (r * 8 + f));
     }
       for(int r = tr + 1, f = tf - 1;  r <= 6 && f >= 1; r++, f--){
        attacks |= (1ULL << (r * 8 + f));
     }
    return attacks;
}
U64 mask_knight_attacks(int square){
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);
     //generate knight attacks 17 15 10 6
    if ( not_h_file & (bitboard >> 17)) attacks  |= (bitboard >> 17); 
    if((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15); 
    if ( not_hg_file & (bitboard >> 10)) attacks  |= (bitboard >> 10); 
    if((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6); 

      if ( not_a_file & (bitboard << 17)) attacks  |= (bitboard << 17); 
    if((bitboard << 15) & not_h_file) attacks |= (bitboard << 15); 
    if ( not_ab_file & (bitboard << 10)) attacks  |= (bitboard << 10); 
    if((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6); 
    return attacks;
}
 U64 mask_pawn_attacks(int square, int side){
     //result attacks
     U64 attacks = 0ULL;
    // piece bitboard 
    U64 bitboard = 0ULL;
    //set piece on board
    set_bit(bitboard, square);
    if(!side){  // -> white
        if ( not_a_file & (bitboard >> 7)) attacks  |= (bitboard >> 7); //shift right capture (make sure we arent on h file)
        if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9); //Same but with left capure (cant capture if we are on the a file)
    }
    else{
            if ( not_h_file & (bitboard << 7)) attacks  |= (bitboard << 7); //shift right capture (make sure we arent on a file)
        if((bitboard << 9) & not_a_file) attacks |= (bitboard << 9); //Same but with left capure (cant capture if we are on the h file)
    }

    //return attack map
    return attacks;
   
 }
 //Create lookup tables for piece attacks
 void MemoAttackTables(){
    // loop over 64 board squares
    for(int square = 0; square < 64; square++){
        // initialize pawn attacks
        pawn_attacks[white][square] = mask_pawn_attacks(square, white);
        pawn_attacks[black][square] = mask_pawn_attacks(square, black);
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
        //bishop_attacks[square] = mask_bishop_attacks(square);
        //rook_attacks[square] = mask_rook_attacks(square);
    }
    
 }
//MAGICS (Appropriate number)
U64 find_magic_number(int square, int relevant_bits, int bishop){
    // initialize occupancies
    U64 occupancies[4096];
    //init attack tables
    U64 attacks[4096];
    //init used attacks
    U64 used_attacks[4096];
    //init attack mask for current piece
    U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);
    //init occupancy indices
    int occupancy_indices = 1 << relevant_bits;
    //loop over occupancy indices
    for(int index = 0; index < occupancy_indices; index++){
            // init occupancies
            occupancies[index] = set_occupancy(index,relevant_bits, attack_mask);
            attacks[index] = bishop ? bishop_attacks_on_fly(square, occupancies[index]) :  rook_attacks_on_fly(square, occupancies[index]);

    }
    // test magic numbers in loop
    for(int random_count = 0; random_count < 1000000000; random_count++){
        //generate magic numnber candidate
        U64 magic_number = generate_candidate_magic_number();
        //skip bad numbers
        if(count_bits((attack_mask * magic_number & 0xFF00000000000000 )) < 6){
            continue;
        }
        //init used attacks
        memset(used_attacks, 0ULL, sizeof(used_attacks));
        //init index & fail flag
        int index, fail;

        //test magic index loop
        for(index = 0, fail = 0; !fail && index < occupancy_indices; index++){
            //init magic index
            int magic_index = (int) ((occupancies[index] * magic_number) >> (64 - relevant_bits));
            //test by  seeing empty index avaialable 
            //if it works, initialize used attacks
            if(used_attacks[magic_index] == 0ULL){
                    used_attacks[magic_index] = attacks[index];
            }
            else if(used_attacks[magic_index] != attacks[index]){
                //doesnt work
                fail = 1;
            }
            if(!fail){
                return magic_number;
            }
        }
        printf(" magic Number didnt work \n");
        return 0ULL;
    }
}
// magic numbers testing
void init_magic_numbers(){
    // loop over 64 board squares
    for(int square = 0; square < 64; square++){
        //init root magic numbers
       // printf(" 0x%llxULL\n", );
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    }
    //printf("\n\n");
     for(int square = 0; square < 64; square++){
        //init bishop magic numbers
     ///   printf(" 0x%llxULL\n", find_magic_number(square, bishop_relevant_bits[square], bishop));
        bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);

    }
}
// init slides attacks
void init_slides_attacks(int bishop){
    //loop over squaress
    for(int square = 0; square < 64; square++){
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);
        //init current masks
        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        // relevant bit count
        int relevant_bits = count_bits(attack_mask);
        //init occupancy indices
        int occupancy_indices = (1 << relevant_bits);
        //loop over indices
        for(int index = 0; index < occupancy_indices; index++){
            //bishop
            if(bishop){
                //initialize current occupancy
                U64 occupancy = set_occupancy(index, relevant_bits, attack_mask);
                // init magic index
                int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
                bishop_attacks[square][magic_index] = bishop_attacks_on_fly(square, occupancy);
            }
            //rook
            else{
                 //initialize current occupancy
                U64 occupancy = set_occupancy(index, relevant_bits, attack_mask);
                // init magic index
                int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_fly(square, occupancy);
            }
        }
    }
   
}
 //init bishop & rooks
    static inline U64 get_bishop_attacks(int square, U64 occupancy){
        //get bishop attacks assuming current board occupancy
        occupancy &= bishop_masks[square];
        occupancy *= bishop_magic_numbers[square];
        occupancy >>= 64 - bishop_relevant_bits[square];
        return bishop_attacks[square][occupancy];
    }
        static inline U64 get_rook_attacks(int square, U64 occupancy){
        //get bishop attacks assuming current board occupancy
        occupancy &= rook_masks[square];
        occupancy *= rook_magic_numbers[square];
        occupancy >>= 64 - rook_relevant_bits[square];
        return rook_attacks[square][occupancy];
    }
    static inline U64 get_queen_attacks(int square, U64 occupancy){
        U64 result = 0ULL;
        //bishop occupancies
         U64 boccupancy = occupancy;
         U64 roccupancy = occupancy;
         //bishop attack tables
         boccupancy &= bishop_masks[square];
        boccupancy *= bishop_magic_numbers[square];
        boccupancy >>= 64 - bishop_relevant_bits[square];
        U64 first  =  bishop_attacks[square][boccupancy];
        //rook attack tables 
          roccupancy &= rook_masks[square];
        roccupancy *= rook_magic_numbers[square];
        roccupancy >>= 64 - rook_relevant_bits[square];
        U64 second = rook_attacks[square][roccupancy];
        result = first |= second;
        return result;
    }
    

//Is the current given square attacked by the current given side?
static inline int is_square_attacked(int square, int side){
    //if a white pawn is being attacked by a black pawn..
    //check is we have a black pawn attack on the square and if that square is being occupied by a white pawn
        if((side == white) && (pawn_attacks[black][ square] & bitboards[P] )){
            return 1;
        }
        //vice versa
         if((side == black) && (pawn_attacks[white][ square] & bitboards[p] )){
            return 1;
        }
        // attacked by knights
        if(knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n] )){
            return 1;
        }
        // attacked by bishops
     if(get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b]) ) return 1;
      // attacked by rooks
     if(get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r]) ) return 1;
      //attacked by queens
      if(get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q]) ) return 1;
        //attacked by kings
        if(king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k] )){
            return 1;
        }
    // by default return not attacked......
    return 0;
}
//print attacked squares
void print_attacked_squares(int side){
    for(int rank = 0; rank < 8; rank++){
        for(int file = 0; file < 8; file++){
            int square = rank * 8 + file;
            //print ranks:
            if(!file){
                printf("  %d", 8 - rank);
            }
            //check whether the square (current) is attacked or nah
            printf(" %d",  (is_square_attacked(square, side)) ? 1 : 0);
        }
        printf("\n");
    }
    printf("\n     a b c d e f g h \n\n\n");
}
/*

     Binary move bits                                                   Hex Converts
0000 0000 0000 0000 0011 1111 source square                             0x3f
0000 0000 0000 1111 1100 0000 target square                             0xfc0
0000 0000 1111 0000 0000 0000 piece                                     0xf000    
0000 1111 0000 0000 0000 0000 promoted piece                            0xf0000
0001 0000 0000 0000 0000 0000 capture flag                              0x100000
0010 0000 0000 0000 0000 0000 double pawn push flag                     0x200000
0100 0000 0000 0000 0000 0000 enpassant capture                         0x400000
1000 0000 0000 0000 0000 0000 castling flag                             0x800000
*/
#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
 (source) | \
 (target << 6) | \
 (piece << 12) | \
 (promoted << 16) | \
 (capture << 20) | \
 (double << 21) |  \
 (enpassant << 22) | \
 (castling << 23)   

 // extract source square
 #define get_move_source(move) (move & 0x3f)
  // extract target square
  #define get_move_target(move) ((move & 0xfc0) >> 6)
  // extract piece
  #define get_move_piece(move) ((move & 0xf000) >> 12)
  // extract promoted piece
  #define get_move_promoted(move) ((move & 0xf0000 ) >> 16)
  // extract captured flag
  #define get_move_capture(move) ((move &  0x100000))
  // extract the double pawn move flag
  #define get_move_double(move) (move & 0x200000)
  // extract the enpassant move flag
  #define get_move_enpassant(move) (move & 0x400000)
  // extract the castling flag
  #define get_move_castling(move) (move & 0x800000)

  // move list structure
  typedef struct{
    // moves
    int moves[256];
    //move count
    int count;

  } moves;
   static inline void add_move(moves * move_list, int move){
    // store move
    move_list -> moves[move_list -> count] = move;
    move_list -> count++;
   }
  // promoted pieces
  char promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n',
  };
  // print move function
  void print_move(int move){
        printf("%s %s  %c \n", s_to_c[get_move_source(move)], s_to_c[get_move_target(move)], promoted_pieces[get_move_promoted(move)]);
  }
    // print move function
  void print_move_list(moves * move_list){
    if(move_list -> count == 0) return;
    printf(" \n    move   piece   capture  double   enpassant   castling \n\n");
    for(int move_count = 0; move_count < move_list -> count; move_count++){
        int move = move_list -> moves[move_count];
        printf("%s %s%c       %s        %d         %d         %d         %d \n", 
        s_to_c[get_move_source(move)], 
        s_to_c[get_move_target(move)],
         get_move_promoted(move) ? promoted_pieces[get_move_promoted(move)] : ' ',
        unicode_pieces[get_move_piece(move)],
        get_move_capture(move) ? 1 : 0,
        get_move_double(move) ? 1 : 0,
        get_move_enpassant(move) ? 1 : 0,
        get_move_castling(move) ? 1 : 0

         );
    }
    printf("Total:  %d", move_list -> count);
  }

// make move on chess board
static inline int make_move(int move, int move_flag){
    // quiet moves
    if(move_flag == all_moves){
            // preserve board state
            copy_board();
            // parse move
            int source_square = get_move_source(move);
            int target_square = get_move_target(move);
             int piece = get_move_piece(move);
            int promoted = get_move_promoted(move);
           int capture = get_move_capture(move);
           int castling =  get_move_castling(move);
           int double_move = get_move_double(move);
           int enpass =  get_move_enpassant(move);

           // move piece
           pop_bit(bitboards[piece], source_square);
           set_bit(bitboards[piece], target_square);
           //hash piece (remove original position, and put in new square)
            hash_key ^= piece_keys[piece][source_square];
            hash_key ^= piece_keys[piece][target_square];
           if(capture){ // if we are capturing something
                //pick up bitboard piece index ranges depending on side
                int start_piece,  end_piece;
                if(side == black){
                        start_piece = P;
                        end_piece = K;
                }
                else{
                    start_piece = p;
                    end_piece = k;
                }
                //loop through
                for(int bit_piece = start_piece; bit_piece <= end_piece; bit_piece++){
                    if(get_bit(bitboards[bit_piece],target_square)){ //if piece is on the target square, remove it (captured)
                           pop_bit(bitboards[bit_piece], target_square); 
                           //remove piece from has key
                           hash_key ^= piece_keys[bit_piece][target_square];
                           break;
                    }
                }
           }
           if(promoted){ // if a piece was promoted
                // erase pawn and make it the piece we want it to be
                pop_bit(bitboards[(side == white) ? P : p], target_square);
                set_bit(bitboards[promoted], target_square);
                hash_key ^= piece_keys[(side == white) ? P : p][target_square];
                hash_key ^= piece_keys[promoted][target_square];
           }
           if(enpass){
            (side == white) ? pop_bit(bitboards[p], target_square + 8) : pop_bit(bitboards[P], target_square - 8);

           if(side == white){
                    hash_key ^= piece_keys[p][target_square + 8];
           }
           else{
                hash_key ^= piece_keys[P][target_square - 8];
           }
           }
            if(enpassant != No_sq) hash_key  ^= enpassant_keys[enpassant];
            enpassant = No_sq;
            //hash Enpassant
           
           if(double_move){
             //handle double push
             (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
             hash_key ^= enpassant_keys[enpassant];

           }
           if(castling){
                switch(target_square){
                    // wk
                    case(g1):
                        pop_bit(bitboards[R], h1);
                        set_bit(bitboards[R], f1);
                        hash_key ^= piece_keys[R][h1]; //remvoe rook from h1
                        hash_key ^= piece_keys[R][f1]; //add rook
                        break;
                    
                    // wq
                       case(c1):
                        pop_bit(bitboards[R], a1);
                        set_bit(bitboards[R], d1);
                          hash_key ^= piece_keys[R][a1]; //remvoe rook from h1
                        hash_key ^= piece_keys[R][d1]; //add rook
                        break;

                    //bk
                          case(g8):
                        pop_bit(bitboards[r], h8);
                        set_bit(bitboards[r], f8);
                          hash_key ^= piece_keys[r][h8]; //remvoe rook from h1
                        hash_key ^= piece_keys[r][f8]; //add rook
                        break;
                    //bq 
                      case(c8):
                        pop_bit(bitboards[r], a8);
                        set_bit(bitboards[r], d8);
                          hash_key ^= piece_keys[r][a8]; //remvoe rook from h1
                        hash_key ^= piece_keys[r][d8]; //add rook
                        break;
                }
           }
           //update hashing for castling
           hash_key ^= castle_key[castle];
           //Update castlign rights
           castle &= castling_rights[source_square];
           castle &= castling_rights[target_square];
            hash_key ^= castle_key[castle];
           //update occupancies
           // first reset occupancies
           memset(occupancies, 0ULL, OCCUPANCY_SIZE);
           // reinit occupancies
           //loop over white piece bitboards
           for(int bb_piece = P; bb_piece <= K; bb_piece++){
            //update white occupancies
            occupancies[white] |= bitboards[bb_piece];
           }
           //same for black
           for(int bb_piece = p; bb_piece <= k; bb_piece++){
            //update black occupancies
            occupancies[black] |= bitboards[bb_piece];
           }
           occupancies[both] |= occupancies[black];
           occupancies[both] |= occupancies[white];
            // change side once we have made a move  use XOR to switch it
            side ^= 1;
            //hash side
            hash_key ^= side_key;
            //------- debug hash key 
            //hashkey for updted position
            U64 hash_from_scratch = generate_hash_key();
            /*
            if(hash_key != hash_from_scratch){
                printf("\n\nMake move \n");
                printf("move: "); 
                print_move(move);
                printf("hash key should be: %llx\n", hash_from_scratch);
                getchar();
            }*/
            //check if the side we just switched from made a move which now put it in check
            if(is_square_attacked( (side == white) ? least_significant_bit_index(bitboards[k]) : least_significant_bit_index(bitboards[K]) ,side)){
                restore_board();
                return 0;
            }
            else{
                return 1;// legal move
            }


    }
    //capture moves
    else{
            // make sure its a capture
            if(get_move_capture(move)){
                make_move(move, all_moves);
            }
            else
            return 0;
    }

    // capture moves
}
// generate all moves
static inline void generate_moves( moves * move_list){
    move_list -> count = 0;
    //init source and target squares
    int source_square,  target_square;
    // current bitboard copy and its attacks
    U64 bitboard, attacks;
    for(int piece = P; piece <=k; piece++){
        // initilalize piece bitboard copy
        bitboard  = bitboards[piece];
    //generate white pawns and kings castling moves
    if(side == white){
        // pickup white pawn index
        if(piece == P){
            // loop over white pawns within white pawn bitboard
            while(bitboard) {
                // init source square
                source_square = least_significant_bit_index(bitboard);
   
                // init target square (one ahdead)
                target_square = source_square - 8;

                //quiet pawn moves 
                if(!(target_square < a8) && !get_bit(occupancies[both], target_square)){
                        // pawn promotion
                        if(source_square >= a7 && source_square <= h7){
                            
                                // add move into a move list
                                /*
                               printf("pawn promotion: %s %sq \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sr \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sb \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_square]);
                               */
                               add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                               add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                               add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                               add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }
                        else{
                             // one square ahead pawn move
                           //  printf("pawn push: %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                                add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                             // 2 square ahead pawn move
                               if((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square - 8)){
                                    int double_push = target_square - 8;
                                   //  printf("pawn push: %s %s \n", s_to_c[source_square], s_to_c[double_push]);
                                      add_move(move_list, encode_move(source_square, double_push, piece, 0, 0, 1, 0, 0));
                               }
                        }
                }
                //init the bitboard for attacks
                attacks = pawn_attacks[side][source_square] & occupancies[black];
                // pawn capture moves
                while(attacks){
                    // initialize target attack square
                    target_square = least_significant_bit_index(attacks);
                     // pawn promotion
                        if(source_square >= a7 && source_square <= h7){
                                // add move into a move list
                                /*
                               printf("pawn capture promotion: %s %sq \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn capture promotion: %s %sr \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn capture promotion: %s %sb \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn  capture promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_square]);
                               */
                                add_move(move_list, encode_move(source_square, target_square, piece,  Q, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }
                        else{
                             // one square ahead pawn move
                            // printf("pawn  capture push: %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                               add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                        }
                    pop_bit(attacks, target_square);
                }
                if( enpassant != No_sq){
                    // look up pawn attacks and bitwise and with enpassant square bits
                    U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                    // make sure enpassant capture available
                    if(enpassant_attacks){
                        //init enpassant capture target square
                        int target_enpassant = least_significant_bit_index(enpassant_attacks);
                        // printf("pawn  enpassant capture promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_enpassant]);
                         add_move(move_list, encode_move(source_square, target_enpassant, piece,  0, 1, 0, 1, 0));

                    }
                }
                //pop the lsb
                pop_bit(bitboard, source_square);
            }
        }
        if (piece == K){
            // king side castling is available....
            if(castle & wk){
                //No pieces between rook and king 
                if(!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1) ) {
                    //make sure king and next square aren't attacked
                    if(!is_square_attacked(e1, black) && !is_square_attacked(f1, black) ){
                     //printf(" castling move: O - O");
                         add_move(move_list, encode_move(e1, g1, piece,  0, 0, 0, 0, 1));
                    }
                }
            }
            if(castle & wq){
                          if(!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1)   && !get_bit(occupancies[both], b1)) {
                    //make sure king and next square aren't attacked
                    if(!is_square_attacked(e1, black) && !is_square_attacked(d1, black)){
                   //  printf(" castling move: O - O - 0");
                    add_move(move_list, encode_move(e1, c1, piece,  0, 0, 0, 0, 1));
                    }
                }
            }
            //queen side castling avaialable....
        }
       }
       // do opposite if black
       else{ 
        if(piece == p){
                  while(bitboard) {
                // init source square
                source_square = least_significant_bit_index(bitboard);
   
                // init target square (one ahdead)
                target_square = source_square + 8;

                //quiet pawn moves 
                if(!(target_square > h1) && !get_bit(occupancies[both], target_square)){
                        // pawn promotion
                        if(source_square >= a2 && source_square <= h2){
                                // add move into a move list
                                /*
                               printf("pawn promotion: %s %sq \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sr \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sb \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_square]);
                               */
                                add_move(move_list, encode_move(source_square, target_square, piece,  q, 0, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else{
                             // one square ahead pawn move
                           //  printf("pawn push: %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                             add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                             // 2 square ahead pawn move
                               if((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square + 8)){
                                    int double_push = target_square + 8;
                                     add_move(move_list, encode_move(source_square, double_push, piece, 0, 0, 1, 0, 0));
                                    // printf("pawn push: %s %s \n", s_to_c[source_square], s_to_c[double_push]);

                               }
                        }

                }
                    //init the bitboard for attacks
                attacks = pawn_attacks[side][source_square] & occupancies[white];
                // pawn capture moves
                while(attacks){
                    // initialize target attack square
                    target_square = least_significant_bit_index(attacks);
                     // pawn promotion
                        if(source_square >= a2 && source_square <= h2){
                                // add move into a move list
                                /*
                               printf("pawn capture promotion: %s %sq \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn capture promotion: %s %sr \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn capture promotion: %s %sb \n", s_to_c[source_square], s_to_c[target_square]);
                               printf("pawn  capture promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_square]);
                               */
                                add_move(move_list, encode_move(source_square, target_square, piece,  q, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                                add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }
                        else{
                             
                            // printf("pawn  capture push: %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                    pop_bit(attacks, target_square);
                }
                if( enpassant != No_sq){
                    // look up pawn attacks and bitwise and with enpassant square bits
                    U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                    // make sure enpassant capture available
                    if(enpassant_attacks){
                        //init enpassant capture target square
                        int target_enpassant = least_significant_bit_index(enpassant_attacks);
                        // printf("pawn  enpassant capture promotion: %s %sn \n", s_to_c[source_square], s_to_c[target_enpassant]);
                           add_move(move_list, encode_move(source_square, target_enpassant, piece,  0, 1, 0, 1, 0));
                    }
                }
                //pop the lsb
                pop_bit(bitboard, source_square);
            }
       }
         if (piece == k){
               
            // king side castling is available....
            if(castle & bk){
                //No pieces between rook and king 
                if(!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8) ) {
                    //make sure king and next square aren't attacked
                    if(!is_square_attacked(e8, white) && !is_square_attacked(f8, white) ){
                    // printf(" castling move: O - O");
                      add_move(move_list, encode_move(e8, g8, piece,  0, 0, 0, 0, 1));
 
                    }
                }
            }
            if(castle & bq){
                          if(!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8)   && !get_bit(occupancies[both], b8)) {
                    //make sure king and next square aren't attacked
                    if(!is_square_attacked(e8, white) && !is_square_attacked(d8, white)){
                   //  printf(" castling move: O - O - 0");
                      add_move(move_list, encode_move(e8, c8, piece,  0, 0, 0, 0, 1));
                    }
                }
            }
            //queen side castling avaialable....
        }
       }
       //generate knight moves
       if((side == white) ? piece == N : piece == n){
        while(bitboard){
            source_square = least_significant_bit_index(bitboard);
            //init piece attacks in order to get a set of target squares
            attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black] );
            //loop over target squares available from generated attacks
            while(attacks){
                
                // initialize target square
                target_square = least_significant_bit_index(attacks);

                //quiet
                if(!get_bit( ((side == white) ? occupancies[black] : occupancies[white]), target_square)){
                   // printf("Knight move %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                     add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                }
                else{
                     // printf("Knight capture %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                      add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                }
                // captures


                pop_bit(attacks, target_square);
            }
            
            
            //pop bit
            pop_bit(bitboard, source_square);
        }
       }

       //generate bishop moves
        if((side == white) ? piece == B : piece == b){
        while(bitboard){
            source_square = least_significant_bit_index(bitboard);
            //init piece attacks in order to get a set of target squares
            attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black] );
            //loop over target squares available from generated attacks
            while(attacks){
                
                // initialize target square
                target_square = least_significant_bit_index(attacks);

                //quiet
                if(!get_bit( ((side == white) ? occupancies[black] : occupancies[white]), target_square)){
                   // printf("Bishop move %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                  add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                }
                else{
                     // printf("Bishop capture %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                     add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                }
                // captures


                pop_bit(attacks, target_square);
            }
            
            
            //pop bit
            pop_bit(bitboard, source_square);
        }
       }
       // generate rook moves
        if((side == white) ? piece == R : piece == r){
        while(bitboard){
            source_square = least_significant_bit_index(bitboard);
            //init piece attacks in order to get a set of target squares
            attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black] );
            //loop over target squares available from generated attacks
            while(attacks){
                
                // initialize target square
                target_square = least_significant_bit_index(attacks);

                //quiet
                if(!get_bit( ((side == white) ? occupancies[black] : occupancies[white]), target_square)){
                   // printf("Rook move %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                 add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                }
                else{
                    //  printf("Rook capture %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                  add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                }
                // captures


                pop_bit(attacks, target_square);
            }
               //pop bit
            pop_bit(bitboard, source_square);
        }
       }
            //Queen Attacks
             if((side == white) ? piece == Q : piece == q){
        while(bitboard){
            source_square = least_significant_bit_index(bitboard);
            //init piece attacks in order to get a set of target squares
            attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black] );
            //loop over target squares available from generated attacks
            while(attacks){
                
                // initialize target square
                target_square = least_significant_bit_index(attacks);

                //quiet
                if(!get_bit( ((side == white) ? occupancies[black] : occupancies[white]), target_square)){
                    //printf("Queen move %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                   add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                }
                else{
                      //printf("Queen capture %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                      add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                }
                // captures


                pop_bit(attacks, target_square);
            }
            
            
            //pop bit
            pop_bit(bitboard, source_square);
        }
       }

       // generate king moves
             if((side == white) ? piece == K : piece == k){
        while(bitboard){
            source_square = least_significant_bit_index(bitboard);
            //init piece attacks in order to get a set of target squares
            attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black] );
            //loop over target squares available from generated attacks
            while(attacks){
                
                // initialize target square
                target_square = least_significant_bit_index(attacks);

                //quiet
                if(!get_bit( ((side == white) ? occupancies[black] : occupancies[white]), target_square)){
                   // printf("King move %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                   add_move(move_list, encode_move(source_square, target_square, piece,  0, 0, 0, 0, 0));
                }
                else{
                     // printf("King capture %s %s \n", s_to_c[source_square], s_to_c[target_square]);
                     add_move(move_list, encode_move(source_square, target_square, piece,  0, 1, 0, 0, 0));
                }
                // captures


                pop_bit(attacks, target_square);
            }
            
            
            //pop bit
            pop_bit(bitboard, source_square);
        }
       }

}
}
//leaf nodes for the perft driver (debugging) will find number of positions reached during testing at a depth
U64  nodes;



// perft driver
static inline void perft_driver(int depth){
    // base case
    if(depth == 0){
        nodes++; // reached the end of depth for a certain move calculation combination
        return;
    }
    //create move list
 moves move_list[1];
 generate_moves(move_list); //generating all moves at a given state
for(int move_count = 0; move_count < move_list -> count; move_count++){
    // preserve board state
    copy_board(); 
    // make move
   if(!make_move(move_list -> moves[move_count], all_moves)) //king was put into check by move  
   continue;
   // we have copied the board now let us recirsively reach the next state
    perft_driver(depth - 1);
    // take back
    restore_board();
      //------- debug hash key 
            //hashkey for updted position
            U64 hash_from_scratch = generate_hash_key();
            /*
            if(hash_key != hash_from_scratch){
                printf("\n\nTake Back \n");
                printf("move: "); 
                print_move(move_list -> moves[move_count]);
                printf("hash key should be: %llx\n", hash_from_scratch);
                getchar();
            }
            */
   // print_board();
    // getchar();
    // time to finish execution
   
 }
}
void perft_test(int depth)
{
    printf("\n     Performance test\n\n");
    
    // create move list instance
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // init start time
    long start = get_time_ms();
    
    // loop over generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {   
        // preserve board state
        copy_board();
        
        // make move
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip to the next move
            continue;
        
        // cummulative nodes
        long cummulative_nodes = nodes;
        
        // call perft driver recursively
        perft_driver(depth - 1);
        
        // old nodes
        long old_nodes = nodes - cummulative_nodes;
        
        // take back
        restore_board();
        
        // print move
        printf("     move: %s%s%c  nodes: %ld\n", s_to_c[get_move_source(move_list->moves[move_count])],
                                                 s_to_c[get_move_target(move_list->moves[move_count])],
                                                 get_move_promoted(move_list->moves[move_count]) ? promoted_pieces[get_move_promoted(move_list->moves[move_count])] : ' ',
                                                 old_nodes);
    }
    
    // print results
    printf("\n    Depth: %d\n", depth);
    printf("    Nodes: %lld\n", nodes);
    printf("     Time: %ld\n\n", get_time_ms() - start);
}

/*Material Score
pawn - 100
knight  - 300
bishop - 350
rook - 500
queen - 1000
king - 10000
*/
int material_score[12] = { //white then black in same order
    100,
    300,
    350,
    500,
    1000,
    10000,
    -100,
    -300,
    -350,
    -500,
    -1000,
    -10000
};
// pawn positional score
const int pawn_score[64] = 
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] = 
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

// bishop positional score
const int bishop_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   20,   0,  10,  10,   0,   20,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

// rook positional score
const int rook_score[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

// king positional score
const int king_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

// mirror positional score tables for opposite side
const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};
/*
          Rank mask            File mask           Isolated mask        Passed pawn mask
        for square a6        for square f2         for square g2          for square c4
     (side by side )                (double pawns)         (pawn islands)        (passed pawns)
    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 1 0 0    8  0 0 0 0 0 1 0 1     8  0 1 1 1 0 0 0 0
    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 1 0 0    7  0 0 0 0 0 1 0 1     7  0 1 1 1 0 0 0 0
    6  1 1 1 1 1 1 1 1    6  0 0 0 0 0 1 0 0    6  0 0 0 0 0 1 0 1     6  0 1 1 1 0 0 0 0
    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 1 0 0    5  0 0 0 0 0 1 0 1     5  0 1 1 1 0 0 0 0
    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 1 0 0    4  0 0 0 0 0 1 0 1     4  0 0 0 0 0 0 0 0
    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 1 0 0    3  0 0 0 0 0 1 0 1     3  0 0 0 0 0 0 0 0
    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 1 0 0    2  0 0 0 0 0 1 0 1     2  0 0 0 0 0 0 0 0
    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 1 0 0    1  0 0 0 0 0 1 0 1     1  0 0 0 0 0 0 0 0

       a b c d e f g h       a b c d e f g h       a b c d e f g h        a b c d e f g h 
*/
//file masks 
U64 rank_masks[64];
U64 file_masks[64];
U64 isolated_pawn_masks[64];
// extract rank from a square [square]
const int get_rank[64] =
{
    7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0
}; 
const int king_safety_bonus = 10;
const int double_pawn_penalty = -10;
const int isolated_pawn_penalty = -10;
const int passed_pawn_bonus[8] = {0, 5, 10, 20, 35, 60, 100, 200};
U64 white_passed_pawn_masks[64];
U64 black_passed_pawn_masks[64];
//semi open file
const int semi_open_file_score = 10;
//open file score
const int open_file_score = 15;
//set file or rank masks
U64 set_file_rank_mask(int file_passed, int rank_passed){
    U64 mask = 0ULL;
    for(int rank = 0; rank < 8; rank++){
        for(int file = 0; file < 8; file++){
            int square = rank * 8 + file;
           // set bit on mask
              if (file_passed != -1)
            {
                // on file match
                if (file == file_passed)
                    // set bit on mask
                    mask |= set_bit(mask, square);
            }
            
            else if (rank_passed != -1)
            {
                // on rank match
                if (rank == rank_passed)
                    // set bit on mask
                    mask |= set_bit(mask, square);
            }
        }
    }

    return mask;
}
void init_evaluation_masks(){
    //init file masks
      for(int rank = 0; rank < 8; rank++){
        for(int file = 0; file < 8; file++){
                int square = rank * 8 + file;
                // file
                file_masks[square] |= set_file_rank_mask(file, -1);
                //rank
                rank_masks[square] |= set_file_rank_mask(-1, rank);
                //isolated
                isolated_pawn_masks[square] |=  (file > 0) ?  set_file_rank_mask(file - 1, -1) : 0;
                isolated_pawn_masks[square] |=  (file < 7) ?  set_file_rank_mask(file + 1, -1) : 0;
           
        }
      }
           for(int rank = 0; rank < 8; rank++){
             for(int file = 0; file < 8; file++){
                     //passed white pawns
                         int square = rank * 8 + file;
                //set file, and 2 adjacent files
                white_passed_pawn_masks[square] |= set_file_rank_mask(file, -1);
                white_passed_pawn_masks[square] |= set_file_rank_mask(file + 1, -1);
                white_passed_pawn_masks[square] |= set_file_rank_mask(file - 1, -1);
                //undo squares up until that rank (only care for what comes after the pawn)
                for(int i = 0; i < (8 - rank); i++){
                    white_passed_pawn_masks[square] &= ~rank_masks[(7 - i) * 8 + file];
                }
          
             
             // black pawns
              //set file, and 2 adjacent files
                black_passed_pawn_masks[square] |= set_file_rank_mask(file, -1);
                black_passed_pawn_masks[square] |= set_file_rank_mask(file + 1, -1);
                black_passed_pawn_masks[square] |= set_file_rank_mask(file - 1, -1);
                 //undo squares up until that rank (only care for what comes after the pawn)
                for(int i = 0; i <= (rank); i++){
                    black_passed_pawn_masks[square] &= ~rank_masks[(i) * 8 + file];
                }
                    
          }
      }
}
// most valuable victim & less valuable attacker

/*
                          
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/
//scooe bounds for mating scores
#define mate_value 49000
#define mate_score 48000
#define INF 50000
// MVV LVA [attacker][victim] -> Used to optimize capture moves based off the attacking piece and the victim piece
//By doing so, we can save a lot of time and not calculate combination of nodes that dont lead to capture, and prioritize those which do
static int mvv_lva[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};
// mex play we can reach within a search
#define MAX_PLY 64
//killer moves [id][ply]
int killer_moves[2][64];
//history moves [piece][square]
int history_moves[12][64];

//Principle variation 
/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5
      
      0    m1   m2   m3   m4   m5   m6
      
      1    0    m2   m3   m4   m5   m6 
      
      2    0    0    m3   m4   m5   m6
      
      3    0    0    0    m4   m5   m6
       
      4    0    0    0    0    m5   m6
      
      5    0    0    0    0    0    m6
*/

//Pv_length[ply]
int pv_length[MAX_PLY];
int pv_table[MAX_PLY][MAX_PLY];
// follow PV & score PW move
int follow_pv, score_pv;
// position evaluation
static inline int evaluate_position(){
    // static evaluation score
    int score = 0;
    //current pieces bitboard copy
    U64 bitboard;
    //init piece and square
    int piece, square;
   
    for(int bb_piece = P; bb_piece <= k; bb_piece++){
        //init piece bitboard copy
        bitboard = bitboards[bb_piece];
        while(bitboard){
            //init piece
            piece = bb_piece;
            //init square
            square = least_significant_bit_index(bitboard);
            //evaluate piece weight
            score += material_score[piece];
            switch (piece){
                // white
                case P: 
                //positional score
                score += pawn_score[square];
                //double pawn penalty
                int num_double_pawns_for_white = count_bits(bitboards[P] & file_masks[square]);;
                if(num_double_pawns_for_white > 1){
                   
                    score += (double_pawn_penalty * num_double_pawns_for_white);
                }
                if((bitboards[P] & isolated_pawn_masks[square]) == 0){
                    score += isolated_pawn_penalty;
                }
                //passed pawn bonus
                if((white_passed_pawn_masks[square] & bitboards[p]) == 0){
                    score += passed_pawn_bonus[get_rank[square]];
                }
                 break;
                case N: score += knight_score[square]; break;
                case B: 
                score += bishop_score[square]; 
                //reward if bishop can access more squares
               score +=  count_bits(get_bishop_attacks(square, occupancies[both]));
                break;
                case R: 
                score += rook_score[square];
                //semi open file bonus
                if((bitboards[P] & file_masks[square]) == 0) {
                 
                    score += semi_open_file_score;
                }
                //open file
                  if(((bitboards[p] | bitboards[P]) & file_masks[square]) == 0) {
                      
                    score += open_file_score;
                }
                 break;
                case K: score += king_score[square]; 
                  //semi open file penalty
                if((bitboards[P] & file_masks[square]) == 0) {
                 
                    score -= semi_open_file_score;
                }
                //open file
                  if(((bitboards[p] | bitboards[P]) & file_masks[square]) == 0) {
                      
                    score -= open_file_score;
                }
                //king safety bonus
                score +=(king_safety_bonus * count_bits(occupancies[white] & king_attacks[square]));
                
                break;
                case Q:
                    //mobility score
                    score +=  count_bits(get_queen_attacks(square, occupancies[both]));
                 break;
                //black
                case p: score -= pawn_score[mirror_score[square]];
                   //double pawn penalty
                  int  num_double_pawns_for_black = count_bits(bitboards[p] & file_masks[square]);
                if(num_double_pawns_for_black > 1){
                    score -= (double_pawn_penalty * num_double_pawns_for_black);
                }
                   if((bitboards[p] & isolated_pawn_masks[square]) == 0){
                    score -= isolated_pawn_penalty;
                }
                 //passed pawn bonus
                if((black_passed_pawn_masks[square] & bitboards[P]) == 0){
                    score -= passed_pawn_bonus[ 7 - get_rank[square]];
                }
                 break;
                case n: score -= knight_score[mirror_score[square]];break;
                case b: score -= bishop_score[mirror_score[square]];
                  //reward if bishop can access more squares
               score -=  count_bits(get_bishop_attacks(square, occupancies[both]));
                break;
                case r: score -= rook_score[mirror_score[square]];
                 //semi open file bonus
                if((bitboards[p] & file_masks[square]) == 0) {
                 
                    score -= semi_open_file_score;
                }
                //open file
                  if(((bitboards[p] | bitboards[P]) & file_masks[square]) == 0) {
                      
                    score -= open_file_score;
                }
                break;
                case k: score -= king_score[mirror_score[square]];
                 //semi open file penalty
                if((bitboards[p] & file_masks[square]) == 0) {
                 
                    score += semi_open_file_score;
                }
                //open file
                  if(((bitboards[p] | bitboards[P]) & file_masks[square]) == 0) {
                      
                    score += open_file_score;
                }
                // king safety bonus
              score -=(king_safety_bonus * count_bits(occupancies[black] & king_attacks[square]));
                break;
                case q:
                 //mobility score
                score -=  count_bits(get_queen_attacks(square, occupancies[both]));
                break;
            }
            //pop ls1b
            pop_bit(bitboard, square);
        }
    }
    
    return (side == white) ? score : -score;
}




/*


        Transposition Table 

*/
#define hash_size 800000
#define hashfEXACT 0
#define hashfALPHA 1
#define hashfBETA 2
typedef struct{
    U64 hash_key; //hash key
    int depth;   // current depth
    int flag;    // Beta, Alpha, or PV move
    int score;
} tt;
// define TT as an instance
tt transposition_tables[hash_size];

//clear hash table
void clear_transposition_table(){
    for(int index = 0; index < hash_size; index++){
        // init everything to 0
       transposition_tables[index].hash_key = 0;
       transposition_tables[index].depth = 0;
       transposition_tables[index].flag = 0;
       transposition_tables[index].score = 0;
    }
}
static inline int read_tt_entry(int alpha, int beta, int depth){
    //create a tt pointer to point to a hash entry of current hash key if available
        tt*  hash_entry = &transposition_tables[hash_key % hash_size];
        if(hash_key == hash_entry -> hash_key){
            // retrieve score independent from path
            // make sure we reach correct depth
            if(hash_entry -> depth >= depth){
                int score = hash_entry -> score; 
                   if (score < -mate_score) score += ply; //Do opposite of store so we get the correct val to return
                  if (score > mate_score) score -= ply;
                // match exact (pv) score 
                if(hash_entry -> flag == hashfEXACT){
                    return score;
                }
                 if(hash_entry -> flag == hashfALPHA && score <= alpha){
                    return alpha;
                }
                if(hash_entry -> flag == hashfBETA && score >= beta){
                    return beta;
                }

            }
        }
        return -100000;
}
static inline void write_tt_entry(int score, int depth, int hash_flag){
          //create a tt pointer to point to a hash entry of current hash key if available
        tt*  hash_entry = &transposition_tables[hash_key % hash_size];
         // store score independent from the actual path
    // from root node (position) to current node (position)
     // -> if the score available from the mated value is within the bounds of a mate score, what we must add the independent path from a position to the mate on the board (if we have a mate in 3, specify so in memoization)
    if (score < -mate_score) score -= ply; 
    if (score > mate_score) score += ply;
        hash_entry -> hash_key = hash_key;
        hash_entry -> flag = hash_flag;
        hash_entry -> depth = depth;
        hash_entry -> score = score;

}
static inline int score_move(int move){
      //if PV move scoring is allowed
      if(score_pv){
          if(pv_table[0][ply] == move){
               score_pv = 0;
                return 20000;
          }
      }
        // score capuremoves
        if(get_move_capture(move)){
            //init target piece
            int target_piece = P;
             int start_piece,  end_piece;
                if(side == black){
                        start_piece = P;
                        end_piece = K;
                }
                else{
                    start_piece = p;
                    end_piece = k;
                }
                //loop through
                for(int bit_piece = start_piece; bit_piece <= end_piece; bit_piece++){
                    if(get_bit(bitboards[bit_piece],get_move_target(move))){ //if piece is on the target square, remove it (captured)
                         target_piece =  bit_piece; 
                           break;
                    }
                }
              //  print_move(move);
              //  printf("\n");
               // printf(" source piece: %c \n", ascii_pieces[get_move_piece(move)]);
                  // printf(" target piece: %c \n", ascii_pieces[target_piece]);
                //score move by MVV LVA to prioritize better captures and quickly prune branches
                //add 10,000 to prioritize over the killer movves and history moves
              return mvv_lva[get_move_piece(move)][target_piece] + 10000;
        }
        else{ // score quiet moves
         //score first killer move
         if(killer_moves[0][ply] == move){
                return 9000;
         }

         //score second killer move
        else if(killer_moves[1][ply] == move){
            return 8000;
         }

         //score history move
         else{
            return history_moves[get_move_piece(move)][get_move_target(move)];
         }

        }
        return 0;
}
typedef struct{
    int x;
    int y;
} Points;

void sort(int lo, int mid, int hi, Points p [] ){
 Points p1[mid - lo + 1];
 Points p2[hi - mid];
 for(int i =0; i < mid - lo + 1; i++){
    p1[i] = p[ lo + i];
 }
 for(int i =0; i < hi - mid; i++){
    p2[i] = p[ mid + 1 + i];
 }
 int  i = 0, j = 0, k = lo;
 while(i < (mid - lo + 1) && j < (hi - mid)){
    if(p1[i].y > p2[j].y){
        p[k] = p1[i];
        k++;
        i++;
    }
    else{
        p[k] = p2[j];
        k++;
        j++;
    }
 }
 while(i < (mid - lo + 1)){
     p[k] = p1[i];
        k++;
        i++;
 }
 while( j < (hi - mid)){
     p[k] = p2[j];
        k++;
        j++;
 }

}
void merge_sort(Points p [], int l, int h){
    if(l < h){
        int mid =  (l + h) / 2;
        merge_sort(p, l, mid);
        merge_sort(p, mid + 1, h);
        sort(l, mid, h, p);
    }
}

static inline int sort_moves(moves * move_list){
    Points point[move_list ->count];
    // move scores
    int move_scores[move_list -> count];
    for(int count = 0; count < move_list -> count; count++){
            //score move and put it into the new move list
           move_scores[count] =  score_move(move_list -> moves[count]);
           Points p;
           p.x = move_list -> moves[count];
           p.y = move_scores[count];
           point[count] = p;
    }
   // Sort points via merge sort
   merge_sort(point, 0, move_list -> count - 1);
   for(int i = 0; i < move_list -> count; i++){
     move_list -> moves[i] = point[i].x;
   }
   return 1; 
}

void print_move_scores(moves * move_list){
printf("     Move Scores: \n\n");
for(int count = 0; count < move_list -> count; count++){
    printf("    move:  ");
    print_move(move_list -> moves[count]);
    printf(" score: %d \n", score_move(move_list -> moves[count]));
}
}
//enable PV move scoring
static inline void enable_pv_scoring(moves * move_list){
    //disable following PV
    follow_pv = 0;
    for(int count = 0; count < move_list -> count; count++){
        //make sure we reference MV move
        if(pv_table[0][ply] == move_list -> moves[count]){
           // allow move scoring & follow pv
            score_pv = 1;

            follow_pv = 1;
        }
    }
}
static inline int is_repetition(){
        
        for(int index = 0; index < repetition_index; index++){
             if(repetition_table[repetition_index] == hash_key){
                        return 1;
             }
        }

    return 0;
}


static inline int quiescence(int alpha, int beta){
    if((nodes & 2047)  == 0){
        // every 2047 nodes we want to listen to User input
        communicate();
    }
    nodes++;
   
   if(ply > MAX_PLY -1) 
   return evaluate_position();
    //evaluate position
    int evaluation = evaluate_position();
    if(evaluation >= beta){
        return beta;
    }
    if(evaluation > alpha){
        alpha = evaluation;
    }
    
      moves move_list[1];
    //generate moves
    generate_moves(move_list);
    //if we are following the principle variation
  
    //sort moves
    sort_moves(move_list);
    //loop over move list and calculate possible negamax
    for(int count = 0; count < move_list -> count; count++){
             
       //preserve board state
       copy_board();
       //increment our ply (side moving)
        ply++;
        repetition_index++;
        repetition_table[repetition_index] = hash_key;
       //make sure only legal mvoes
       if(make_move(move_list -> moves[count], only_captues) == 0){
        //decrement ply
        ply--;
         repetition_index--;
        continue;
       }
       //score current move
       int score = -quiescence(-beta, -alpha);
       //take the move back again
       restore_board();
       ply--;
       repetition_index--;
           if(stopped == 1){
            return 0; // time is up
        }
       //fail hard beta cutoff
     
       //found a better move for us
       if(score > alpha){
      
        //pv node(move)
        alpha = score;
          if(score >= beta){
        return beta; //prune search, if we have another pathway that has a better move for the opposite player the bot assumes that pathway is taken
       }
       }
    }
    return alpha;
}
//Late Move Reduction optimizers
const int FullDepthMoves = 4;
const int ReductionLimit = 3;
//negamax alpha beta search
// negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth)
{
    // variable to store current move's score (from the static evaluation perspective)
    int score;
    
    // define hash flag
    int hash_flag = hashfALPHA;
    
    // if position repetition occurs
   if(ply && is_repetition()) return 0;
    
    // a hack by Pedro Castro to figure out whether the current node is PV node or not 
    int pv_node = (beta - alpha)> 1;
    
    // read hash entry if we're not in a root ply and hash entry is available
    if ( ply && (score = read_tt_entry(alpha, beta, depth)!= -100000))
        // if the move has already been searched (hence has a value)
        // we just return the score for this move without searching it
        return score;
        
    // every 2047 nodes
    if((nodes & 2047 ) == 0)
        // "listen" to the GUI/user input
        communicate();

    // init PV length
    pv_length[ply] = ply;

    // recursion escape condition
    if (depth == 0)
        // run quiescence search
        return quiescence(alpha, beta);
    
    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (ply > MAX_PLY - 1)
        // evaluate position
        return evaluate_position();
    
    // increment nodes count
    nodes++;
    
    // is king in check
    int in_check = is_square_attacked((side == white) ? least_significant_bit_index(bitboards[K]) : 
                                                        least_significant_bit_index(bitboards[k]),
                                                        side ^ 1);
    
    // increase search depth if the king has been exposed into a check
    if (in_check) depth++;
    
    // legal moves counter
    int legal_moves = 0;
    
    // null move pruning
    if (depth >= 3 && in_check == 0 && ply)
    {
        // preserve board state
        copy_board();
        
        // increment ply
        ply++;
        
        // increment repetition index & store hash key
       repetition_index++;
        repetition_table[repetition_index] = hash_key;

        
        // hash enpassant if available
        if (enpassant != No_sq) hash_key ^= enpassant_keys[enpassant];
        
        // reset enpassant capture square
        enpassant = No_sq;
        
        // switch the side, literally giving opponent an extra move to make
        side ^= 1;
        
        // hash the side
        hash_key ^= side_key;
                
        /* search moves with reduced depth to find beta cutoffs
           depth - 1 - R where R is a reduction limit */
        score = -negamax(-beta, -beta + 1, depth - 1 - 2);
        
        // decrement repetition index
        //repetition_index--;
                ply--;
        repetition_index--;
        // restore board state
        restore_board();
       // decrement ply
    
        // reutrn 0 if time is up
        if(stopped == 1) return 0;

        // fail-hard beta cutoff
        if (score >= beta)
            // node (position) fails high
            return beta;
    }
    
    // create move list instance
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // if we are now following PV line
    if (follow_pv)
        // enable PV move scoring
        enable_pv_scoring(move_list);
    
    // sort moves
    sort_moves(move_list);
    
    // number of moves searched in a move list
    int moves_searched = 0;
    
    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++)
    {
        // preserve board state
        copy_board();
        
        // increment ply
        ply++;
        repetition_index++;
        repetition_table[repetition_index] = hash_key;
        
        // increment repetition index & store hash key
      //  repetition_index++;
        //repetition_table[repetition_index] = hash_key;
        
        // make sure to make only legal moves
        if (make_move(move_list->moves[count], all_moves) == 0)
        {
            // decrement ply
            ply--;
            
            // decrement repetition index
            repetition_index--;
            
            // skip to next move
            continue;
        }
        
        // increment legal moves
        legal_moves++;
        
        // full depth search
        if (moves_searched == 0)
            // do normal alpha beta search
            score = -negamax(-beta, -alpha, depth - 1);
        
        // late move reduction (LMR)
        else
        {
            // condition to consider LMR
            if(
                moves_searched >= FullDepthMoves &&
                depth >= ReductionLimit &&
                in_check == 0 && 
                get_move_capture(move_list->moves[count]) == 0 &&
                get_move_promoted(move_list->moves[count]) == 0
              )
                // search current move with reduced depth:
                score = -negamax(-alpha - 1, -alpha, depth - 2);
            
            // hack to ensure that full-depth search is done
            else score = alpha + 1;
            
            // principle variation search PVS
            if(score > alpha)
            {
             /* Once you've found a move with a score that is between alpha and beta,
                the rest of the moves are searched with the goal of proving that they are all bad.
                It's possible to do this a bit faster than a search that worries that one
                of the remaining moves might be good. */
                score = -negamax(-alpha - 1, -alpha, depth-1);
            
             /* If the algorithm finds out that it was wrong, and that one of the
                subsequent moves was better than the first PV move, it has to search again,
                in the normal alpha-beta manner.  This happens sometimes, and it's a waste of time,
                but generally not often enough to counteract the savings gained from doing the
                "bad move proof" search referred to earlier. */
                if((score > alpha) && (score < beta))
                 /* re-search the move that has failed to be proved to be bad
                    with normal alpha beta score bounds*/
                    score = -negamax(-beta, -alpha, depth-1);
            }
        }
        
        // decrement ply
        ply--;
        
        // decrement repetition index
       repetition_index --;

        // take move back
        restore_board();
        
        // reutrn 0 if time is up
        if(stopped == 1) return 0;
        
        // increment the counter of moves searched so far
        moves_searched++;
        
        // found a better move
        if (score > alpha)
        {
            // switch hash flag from storing score for fail-low node
            // to the one storing score for PV node
            hash_flag = hashfEXACT;
        
            // on quiet moves
            if (get_move_capture(move_list->moves[count]) == 0)
                // store history moves
                history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;
            
            // PV node (position)
            alpha = score;
            
            // write PV move
            pv_table[ply][ply] = move_list->moves[count];
            
            // loop over the next ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                // copy move from deeper ply into a current ply's line
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            
            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];
            
            // fail-hard beta cutoff
            if (score >= beta)
            {
                // store hash entry with the score equal to beta
                write_tt_entry(beta, depth, hashfBETA);
            
                // on quiet moves
                if (get_move_capture(move_list->moves[count]) == 0)
                {
                    // store killer moves
                    killer_moves[1][ply] = killer_moves[0][ply];
                    killer_moves[0][ply] = move_list->moves[count];
                }
                
                // node (position) fails high
                return beta;
            }            
        }
    }
    
    // we don't have any legal moves to make in the current postion
    if (legal_moves == 0)
    {
        // king is in check
        if (in_check)
            // return mating score (assuming closest distance to mating position) mate position in 8 plies is -49000 + 8 
            return -mate_value + ply;
        
        // king is not in check
        else
            // return stalemate score
            return 0;
    }
    
    // store hash entry with the score equal to alpha
        write_tt_entry(alpha, depth, hash_flag);
    
    // node (position) fails low
    return alpha;
}
//search position for best move
void search_position(int depth)
{
   // printf("%d \n", depth);
    // define best score variable
    int score = 0;
    
    // reset nodes counter
    nodes = 0;
    
    // reset "time is up" flag
    stopped = 0;
    
    // reset follow PV flags
    follow_pv = 0;
    score_pv = 0;
    
    // clear helper data structures for search
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_table, 0, sizeof(pv_table));
    memset(pv_length, 0, sizeof(pv_length));
    
    // define initial alpha beta bounds
    int alpha = -INF;
    int beta = INF;
 
    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {
        // if time is up
        if(stopped == 1)
			// stop calculating and return best move so far 
			break;
		
        // enable follow PV flag
        follow_pv = 1;
        
        // find best move within a given position
        score = negamax(alpha, beta, current_depth);
 
        // we fell outside the window, so try again with a full-width window (and the same depth)
        if ((score <= alpha) || (score >= beta)) {
            alpha = -INF;    
            beta = INF;      
            continue;
        }
        
        // set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;
        
        // print search info
        if (score > -mate_value && score < -mate_score)
            printf("info score mate %d depth %d nodes %lld time %d pv ", -(score + mate_value) / 2 - 1, current_depth, nodes, get_time_ms() - starttime);
        
        else if (score > mate_score && score < mate_value)
            printf("info score mate %d depth %d nodes %lld time %d pv ", (mate_value - score) / 2 + 1, current_depth, nodes, get_time_ms() - starttime);   
        
        else
            printf("info score cp %d depth %d nodes %lld time %d pv ", score, current_depth, nodes, get_time_ms() - starttime);
        
        // loop over the moves within a PV line
        for (int count = 0; count < pv_length[0]; count++)
        {
            // print PV move
            print_move(pv_table[0][count]);
            printf(" ");
        }
        
        // print new line
        printf("\n");
    }

    // print best move
    printf("bestmove ");
    print_move(pv_table[0][0]);
    printf("\n");
}

//UCI
//parse string input
int parse_move(char * move_string){
    // create move list
    moves move_list[1];
    generate_moves(move_list);
    //parse moves 
    //source square
    int source_square = (move_string[0]  -'a') + (8 -(move_string[1] - '0')) * 8;
    //target square
     int target_square = (move_string[2]  -'a') + (8 -(move_string[3] - '0')) * 8;
    
     // loop over move list to find possible promotions
     for(int gen_move = 0; gen_move < move_list -> count; gen_move++){
        //init move
        int move = move_list -> moves[gen_move];
        //mkae sure source and target square are available within generated move
        if(source_square == get_move_source(move) && target_square == get_move_target(move)){
             //init pronoted piece
         int promoted_piece = get_move_promoted(move);
        // printf("  %d \n", promoted_piece);
         if(promoted_piece) {
          //  printf("here    %c", move_string[4]);
                //if contains promoted or not...
                    if((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q')
                //legal move
                return move;
                else if((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n')
                //legal move
                return move;
               else  if((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b')
                //legal move
                return move;
              else   if((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r')
                //legal move
                return move;
                
                continue;
         }
         return move;
        }
     }
      //return illegal if not part of move list
      return 0;
}

/*

UCI Commands to init a chess board:
setup position, then call search
commands such as startpos moves e2e4
you shoul be able to populate fens
combination of both
*/
//Parse UCI position command
void parse_position(char * command){
    //shift pointer to the right where the token begins
    command += 9;
    //init pointer to cur char in command string
    char * current_char = command;
    //parse UCI "startpos" command
    if(strncmp(command, "startpos", 8) == 0){ //our command is startpos (so we want to parse the starting position
       parse_fen(start_position);
       //  printf(" %s \n", current_char);
    }
    else{ // its gonna be fen command (so we are parsing some custom string)
    current_char = strstr(command, "fen");
    if(current_char == NULL) {
        parse_fen(start_position); //if not there just do starting position
    }
    else{
        //Shift pointer to the right to go past fen and the auxiallary space and go to the value it holds
        current_char += 4;
        parse_fen(current_char); //parse custom character

    } 
    }  

      current_char = strstr(command, "moves"); //mext command is moves
   
        //check if it is there
        if(current_char != NULL){
           // shift to right spot
           printf("here \n");
           current_char +=6;
           while(*current_char){
                //parse next move
                int move = parse_move(current_char);
                if(move == 0){
                    break;
                }
                make_move(move, all_moves);
                //current hash key is our move for this ply
                repetition_table[repetition_index] = hash_key;
                repetition_index++;

                while(*current_char && *current_char != ' '){
                    current_char++;
                }
                current_char++; // go to next move :) im in the airport to colorado rn 
           }
              
          
           // next move
        }
        print_board();
    
}
//parse go - go depth 6
// parse UCI command "go"

void parse_go(char *command)
{
    // init parameters
    int depth = -1;

    // init argument
    char *argument = NULL;

    // infinite search
    if ((argument = strstr(command,"infinite"))) {}

    // match UCI "binc" command
    if ((argument = strstr(command,"binc")) && side == black)
        // parse black time increment
        inc = atoi(argument + 5);

    // match UCI "winc" command
    if ((argument = strstr(command,"winc")) && side == white)
        // parse white time increment
        inc = atoi(argument + 5);

    // match UCI "wtime" command
    if ((argument = strstr(command,"wtime")) && side == white)
        // parse white time limit
        times = atoi(argument + 6);

    // match UCI "btime" command
    if ((argument = strstr(command,"btime")) && side == black)
        // parse black time limit
        times = atoi(argument + 6);

    // match UCI "movestogo" command
    if ((argument = strstr(command,"movestogo")))
        // parse number of moves to go
        movestogo = atoi(argument + 10);

    // match UCI "movetime" command
    if ((argument = strstr(command,"movetime")))
        // parse amount of time allowed to spend to make a move
        movetime = atoi(argument + 9);

    // match UCI "depth" command
    if ((argument = strstr(command,"depth")))
        // parse search depth
        depth = atoi(argument + 6);

    // if move time is not available
    if(movetime != -1)
    {
        // set time equal to move time
        times = movetime;

        // set moves to go to 1
        movestogo = 1;
    }

    // init start time
    starttime = get_time_ms();

    // init search depth
    depth = depth;

    // if time control is available
    if(times != -1)
    {
        // flag we're playing with time control
        timeset = 1;

        // set up timing
        times /= movestogo;
        if(times > 1500) times -= 50;
        times -= 50;
        stoptime = starttime + times + inc;
    }

    // if depth is not available
    if(depth == -1)
        // set depth to 64 plies (takes ages to complete...)
        depth = 64;

    // print debug info
    printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
    time, starttime, stoptime, depth, timeset);
     
    // search position
    search_position(depth);
}
/*
GUI  -> isready     readyok <- Engine
GUI -> ucinewgame (start new game)


*/
//main uci loop
void uci_loop(){
    // reset the STDIN and STDOUT buffers
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    //define use anr gui input buffer
    char input[2000];
    //print identifying engine info
    printf("id name YabyerChessEngine \n");
    printf("id name Yabyer \n");
    printf("uciok \n");
    
    //main loop
    while(1){
        // reset user / GUI input
        memset(input, 0, sizeof(input));
        //makae sure output reaches the GUI
        fflush(stdout);
        if(!fgets(input, 2000, stdin)){
            //continue if we get something
            continue;
        }
        //make sure input is available
        if(input[0] == '\n'){
            continue;
        }
        
        //if GUI says "isready" it means we can send it input to display, so we can send back "readyok" from the engine to convey that
        if(strncmp(input, "isready", 7) == 0){
            printf("readyok \n");
            continue;
        }
        //parse uci "position"
        if(strncmp(input, "position", 8) == 0){
            //parse the position
            parse_position(input);
                clear_transposition_table();
        }
        //parse UCI newgame command
       else if(strncmp(input, "ucinewgame", 10) == 0){
            parse_position("position startpos");
            clear_transposition_table();
        }
        //parse UCI go
       else   if(strncmp(input, "go", 2) == 0){
            parse_go(input);
        }
        //parse UCI quit
        else  if(strncmp(input, "quit", 4) == 0){
            //exit
            return;
        }
        //parse UCI "uci" command
       else   if(strncmp(input, "uci", 3) == 0){
            printf("id name YabyerChessEngine \n");
           printf("id author Yabyer \n");
           printf("uciok \n");
        }
    }

}
//d191dfd63f2bbd9b
//d191dfd63f2bbd9b
//init all variables
void init_all(){
       MemoAttackTables();
       init_slides_attacks(bishop);
        init_slides_attacks(rook);
        init_rand_keys();
        clear_transposition_table();
        init_evaluation_masks();
}
int main(){
    // define bitboards
    
  init_all();
    uci_loop();
 
    return 0;
   
}