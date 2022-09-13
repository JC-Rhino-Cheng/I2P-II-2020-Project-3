#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

struct Point {
    int x, y;
    Point() : Point(0, 0) {}
    Point(int x, int y) : x(x), y(y) {}
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
};

int player;
const int SIZE = 8;
std::array<std::array<int, SIZE>, SIZE> board;
std::array<std::array<int, SIZE>, SIZE> c;
std::vector<Point> next_valid_spots;

class Othello
{
public:
    std::array<std::array<int, SIZE>, SIZE> chessboard;
    std::vector<Point> valid_move;
    int cur_player;
    const std::array<Point, 8> directions{{
        Point(-1, -1), Point(-1, 0), Point(-1, 1),
        Point(0, -1), /*{0, 0}, */Point(0, 1),
        Point(1, -1), Point(1, 0), Point(1, 1)
    }};
    Othello(std::array<std::array<int, SIZE>, SIZE> board,Point p)
    :cur_player(player)
    {
        chessboard=flip_discs(p, board);
    }
    
    Othello(const Othello &s)
    :chessboard(s.chessboard),cur_player(s.cur_player){}
    
    bool is_spot_on_board(Point p) const {
        return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
    }
    
    bool is_disc_at(Point p, int disc) const {
        if (!is_spot_on_board(p))
            return false;
        if (chessboard[p.x][p.y] != disc)
            return false;
        return true;
    }
    
    bool is_spot_valid(Point center) const {
        if (chessboard[center.x][center.y] != 0)
            return false;
        for (Point dir: directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, 3-cur_player))
                continue;
            p = p + dir;
            while (is_spot_on_board(p) && chessboard[p.x][p.y] != 0) {
                if (is_disc_at(p, cur_player))
                    return true;
                p = p + dir;
            }
        }
        return false;
    }
    
    std::vector<Point> find_valid_move()
    {
        std::vector<Point> valid_spots;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                Point p = Point(i, j);
                if (chessboard[i][j] != 0)
                    continue;
                if (is_spot_valid(p))
                    valid_spots.push_back(p);
            }
        }
        return valid_spots;
    }
    
    std::array<std::array<int, SIZE>, SIZE> flip_discs(Point center,std::array<std::array<int, SIZE>, SIZE> board) {
        std::array<std::array<int, SIZE>, SIZE> newboard=board;
        newboard[center.x][center.y]=cur_player;
        for (Point dir: directions) {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, 3-cur_player))
                continue;
            std::vector<Point> discs({p});
            p = p + dir;
            while (is_spot_on_board(p) && chessboard[p.x][p.y] != 0) {
                if (is_disc_at(p, cur_player)) {
                    for (Point s: discs) {
                        newboard[s.x][s.y]=cur_player;
                    }
                    break;
                }
                discs.push_back(p);
                p = p + dir;
            }
        }
        return newboard;
    }
    
    int evaluation()
    {
        int score=0;
        for(int i=0;i<SIZE;i++)
            for(int j=0;j<SIZE;j++){
                if(chessboard[i][j]==player){
                    score++;
                    if(i==0 || i==7 || j==0 || j==7){
                        score+=20;
                        if(i==j)
                            score+=20;
                    }
                }
                else if(chessboard[i][j]==3-player){
                    if(i==0 || i==7 || j==0 || j==7){
                        if(i==j)
                            score-=40;
                    }
                }
            }
        return score;
    }
    
    
    int minimax(int depth,int alpha,int beta)
    {
        if(depth==0)
            return evaluation();
        valid_move=find_valid_move();
        int maxeval=-999999,mineval=999999,eval;
        for(auto c:valid_move){
            Othello n=*this;
            n.chessboard=flip_discs(c, n.chessboard);
            n.cur_player=3-cur_player;
            eval=n.minimax(depth-1, alpha, beta);
            if(cur_player==player){
                maxeval=std::max(eval, maxeval);
                alpha=std::max(alpha,eval);
            }
            else{
                mineval=std::min(eval,mineval);
                beta=std::min(eval,beta);
            }
            if(beta<=alpha)
                break;
        }
        if(cur_player==player)
            return maxeval;
        else
            return mineval;
    }
};

void read_board(std::ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
}

void read_valid_spots(std::ifstream& fin) {
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++) {
        fin >> x >> y;
        next_valid_spots.push_back({x, y});
    }
}

void write_valid_spot(std::ofstream& fout) {
    Point best_move=next_valid_spots.front();
    int max=-999999;
    for(auto c:next_valid_spots){
        Othello n(board,c);
        int val=n.minimax(6, -999999, 999999);
        if(val>max){
            max=val;
            best_move=c;
        }
    }
    fout << best_move.x << " " << best_move.y << std::endl;
    fout.flush();
}

int main(int, char** argv) {
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}


