#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>

struct Point {
	int x, y;
	Point() : Point(0, 0) {}
	Point(float x, float y) : x(x), y(y) {}
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
std::array<std::array<int, SIZE>, SIZE> brd;
std::vector<Point> next_valid_spots;

void read_board(std::ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> brd[i][j];
        }
    }
}

void read_valid_spots(std::ifstream& fin) {
	int n_valid_spots;
	fin >> n_valid_spots;
	int x, y;
	for (int i = 0; i < n_valid_spots; i++) {
		fin >> x >> y;
		next_valid_spots.push_back({ (float)x, (float)y });
	}
}

void write_valid_spot(std::ofstream& fout);

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


class OthelloBoard {
public:
	enum SPOT_STATE {
		EMPTY = 0,
		BLACK = 1,
		WHITE = 2
	};
	static const int SIZE = 8;
	const std::array<Point, 8> directions{ {
		Point(-1, -1), Point(-1, 0), Point(-1, 1),
		Point(0, -1), /*{0, 0}, */Point(0, 1),
		Point(1, -1), Point(1, 0), Point(1, 1)
	} };
	std::array<std::array<int, SIZE>, SIZE> board;
	std::vector<Point> next_valid_spots;
	std::array<int, 3> disc_count;
	int cur_player;
	bool done;
	int winner;
private:
	//透過傳入目前的player編號，抓取下一個player編號
	int get_next_player(int player) const {
		return 3 - player;
	}
	//檢查是否超出螢幕有效範圍
	bool is_spot_on_board(Point p) const {
		return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
	}
	//把某一個特定座標所具有的disc編號傳回。可能的值: 0-EMPTY、1-BLACK、2-WHITE
	int get_disc(Point p) const {
		return board[p.x][p.y];
	}
	//把某一個特定座標修改為特定的disc編號。可能的值: 0-EMPTY、1-BLACK、2-WHITE
	//不必用到，因為後面會再外包給put_disc執行
	void set_disc(Point p, int disc) {
		board[p.x][p.y] = disc;
	}
	//輸入位置座標和想要的disc編號，檢查是否確實在那個位置有著想要的disc
	bool is_disc_at(Point p, int disc) const {
		if (!is_spot_on_board(p))
			return false;
		if (get_disc(p) != disc)
			return false;
		return true;
	}
	//在想要把一個棋子下下去一個位置之前，所做的檢查。做了以下幾件事情:
	//(1)先檢查該點是否已經不可下下去
	//(2)往八個方向檢查，看看是否有同樣顏色的東西在做末端有包圍住，以供有效的下法。
	bool is_spot_valid(Point center) const {
		if (get_disc(center) != EMPTY)
			return false;
		for (Point dir : directions) {
			// Move along the direction while testing.
			Point p = center + dir;
			if (!is_disc_at(p, get_next_player(cur_player)))
				continue;
			p = p + dir;
			while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
				if (is_disc_at(p, cur_player))
					return true;
				p = p + dir;
			}
		}
		return false;
	}
	//下下一個棋子之後，往八個方位進行檢查，看看如果這個方位確實是可以被flip的話，先用list記下來需要flip的座標，之後一個方位檢查後統一處理。之後，disc_count相關的資訊一併調整
	//不必使用，因為會外包給put_disc使用
	void flip_discs(Point center) {
		for (Point dir : directions) {
			// Move along the direction while testing.
			Point p = center + dir;
			if (!is_disc_at(p, get_next_player(cur_player)))
				continue;
			std::vector<Point> discs({ p });
			p = p + dir;
			while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
				if (is_disc_at(p, cur_player)) {
					for (Point s : discs) {
						set_disc(s, cur_player);
					}
					disc_count[cur_player] += discs.size();
					disc_count[get_next_player(cur_player)] -= discs.size();
					break;
				}
				discs.push_back(p);
				p = p + dir;
			}
		}
	}
public:
	//Constructor
	OthelloBoard() {
		reset();
	}
	//Copy Constructor
	OthelloBoard(const OthelloBoard & rhs) :cur_player(rhs.cur_player), done(rhs.done), winner(rhs.winner) {
		for (auto i = 0; i < rhs.SIZE; i++) for (auto j = 0; j < rhs.SIZE; j++) {
			board[i][j] = rhs.board[i][j];
		}

		for (auto i = 0; i < 3; i++)disc_count[i] = rhs.disc_count[i];
	}
	//從現有的2D盤面建構Board
	OthelloBoard(std::array<std::array<int, SIZE>, SIZE> brd, int player_idx) : done(false), winner(-1), cur_player(player_idx) {
		disc_count[EMPTY] = disc_count[BLACK] = disc_count[WHITE] = 0;
		
		for (int i = 0; i < SIZE; i++)for (int j = 0; j < SIZE; j++) {
			board[i][j] = brd[i][j];

			disc_count[board[i][j]]++;//是什麼棋子，就把那個棋子的count做++。
		}

	}
	//把盤面還原到最初始的棋盤
	void reset() {
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				board[i][j] = EMPTY;
			}
		}
		board[3][4] = board[4][3] = BLACK;
		board[3][3] = board[4][4] = WHITE;
		cur_player = BLACK;
		disc_count[EMPTY] = 8 * 8 - 4;
		disc_count[BLACK] = 2;
		disc_count[WHITE] = 2;
		next_valid_spots = get_valid_spots();
		done = false;
		winner = -1;
	}
	//在一個有效位置被下下去之後，更新所有新的valid spots。
	std::vector<Point> get_valid_spots() const {
		std::vector<Point> valid_spots;
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				Point p = Point(i, j);
				if (board[i][j] != EMPTY)
					continue;
				if (is_spot_valid(p))
					valid_spots.push_back(p);
			}
		}
		return valid_spots;
	}
	//如果下的位置是錯的，那麼贏家是誰就出爐了。否則的話就是更新valid spots。而如果現在的valid spots是0個，而再下一輪還是0個的話，代表兩個玩家都已經沒有路可以走了，意味著遊戲已經結束。
	bool put_disc(Point p) {
		if (!is_spot_valid(p)) {
			winner = get_next_player(cur_player);
			done = true;
			return false;
		}
		set_disc(p, cur_player);
		disc_count[cur_player]++;
		disc_count[EMPTY]--;
		flip_discs(p);
		// Give control to the other player.
		cur_player = get_next_player(cur_player);
		next_valid_spots = get_valid_spots();
		// Check Win
		if (next_valid_spots.size() == 0) {
			cur_player = get_next_player(cur_player);
			next_valid_spots = get_valid_spots();
			if (next_valid_spots.size() == 0) {
				// Game ends
				done = true;
				int white_discs = disc_count[WHITE];
				int black_discs = disc_count[BLACK];
				if (white_discs == black_discs) winner = EMPTY;
				else if (black_discs > white_discs) winner = BLACK;
				else winner = WHITE;
			}
		}
		return true;
	}
	
	int MiniMax(int cur_depth, int terminal_depth);
};




void write_valid_spot(std::ofstream& fout) {
	int n_valid_spots = next_valid_spots.size();//總共有這麼多可以下的盤面

	OthelloBoard curState(brd, player);//先把目前main.exe所已經提供的盤面建構出來。題目跟我講我現在要下的是player(ex: 這裡以及以下的例子都用我想要下白色來說明)

	for (int terminal_depth = 0; terminal_depth < 6; terminal_depth++) {
		std::vector<int>State_Value;//紀錄每個valid下法的好壞程度
		State_Value.resize(n_valid_spots, 0);

		for (int i = 0; i < n_valid_spots; i++) {
			Point p = next_valid_spots[i];

			OthelloBoard temp = curState;
			temp.put_disc(p);//我已經把白色棋子下下去了。而目前temp裡的cur_player會自動改成黑色。
			State_Value[i] = temp.MiniMax(0, terminal_depth);
		}
		//開始輸出最佳的放法
		//因為准許無限次輸出，所以我只要每找到一個比較好的放法，就輸出一次，即可
		//記得區分現在的player到底是BLACK還是WHITE
		if (player == curState.BLACK) {
			int now_MAX = INT_MIN;
			for (int i = 0; i < n_valid_spots; i++)if (State_Value[i] > now_MAX) {
				now_MAX = State_Value[i];

				fout << next_valid_spots[i].x << " " << next_valid_spots[i].y << std::endl;
				fout.flush();
			}
		}
		else if (player == curState.WHITE) {
			//因為我下的是白色的，所以State_Value裡面儲存的是我每個下法對「黑色」來講有多糟，所以我只要取對黑色來講最糟的(數值最小的)，就是對我白色最好的
			int now_min = INT_MAX;
			for (int i = 0; i < n_valid_spots; i++)if (State_Value[i] < now_min) {
				now_min = State_Value[i];

				fout << next_valid_spots[i].x << " " << next_valid_spots[i].y << std::endl;
				fout.flush();
			}
		}
	}
	
	
	











	
}

const std::array<Point, 4> Corners{ {
		Point(0, 0), Point(0, SIZE - 1),
		Point(SIZE - 1, 0),Point(SIZE - 1, SIZE - 1)
		//左上、右上
		//左下、右下
} };

const std::array<Point, 4> Corners_Inner{ {
		Point(Corners[0].x + 1, Corners[0].y + 1), Point(Corners[1].x + 1, Corners[1].y - 1),
		Point(Corners[2].x - 1, Corners[2].y + 1), Point(Corners[3].x - 1, Corners[3].y - 1)
} };

const std::array<Point, 8> Corners_two_sides{ {
		Point(Corners[0].x + 1, Corners[0].y),
		Point(Corners[0].x , Corners[0].y + 1),
		Point(Corners[1].x + 1, Corners[1].y),
		Point(Corners[1].x , Corners[1].y - 1),
		Point(Corners[2].x - 1, Corners[2].y),
		Point(Corners[2].x , Corners[2].y + 1),
		Point(Corners[3].x - 1, Corners[3].y),
		Point(Corners[3].x , Corners[3].y - 1)
} };


int OthelloBoard::MiniMax(int cur_depth, int terminal_depth) {
	//先計算目前的valid的格子有哪些
	//不管是if(cur_depth == terminal_depth)還是後面兩個，都會用到
	next_valid_spots = get_valid_spots();
	
	if (cur_depth == terminal_depth) {
		int score = 0;

		//以下的line334~350考慮的是valid_spots的部分。之後的lin353~會考慮場上已經有的棋子數的部分
		int num_of_to_be_put_corners = 0;
		int num_of_to_be_put_corners_inner = 0;
		int num_of_to_be_put_corners_two_sides = 0;
		int num_of_to_be_put_remainings = 0;
		for (int i = 0; i < next_valid_spots.size(); i++) {
			if (next_valid_spots[i] == Corners[0] || next_valid_spots[i] == Corners[1] || next_valid_spots[i] == Corners[2] || next_valid_spots[i] == Corners[3])
				num_of_to_be_put_corners++;
			else if (next_valid_spots[i] == Corners_Inner[0] || next_valid_spots[i] == Corners_Inner[1] || next_valid_spots[i] == Corners_Inner[2] || next_valid_spots[i] == Corners_Inner[3])
				num_of_to_be_put_corners_inner++;
			else if (
				next_valid_spots[i] == Corners_two_sides[0] || next_valid_spots[i] == Corners_two_sides[1] ||
				next_valid_spots[i] == Corners_two_sides[2] || next_valid_spots[i] == Corners_two_sides[3] ||
				next_valid_spots[i] == Corners_two_sides[4] || next_valid_spots[i] == Corners_two_sides[5] ||
				next_valid_spots[i] == Corners_two_sides[6] || next_valid_spots[i] == Corners_two_sides[7])
				num_of_to_be_put_corners_two_sides++;
			else num_of_to_be_put_remainings++;
		}

		//接下來考慮的是原本已經有的棋盤上的黑棋白棋的數量
		//以下陣列的編號0都作廢。1代表黑色，2代表白色
		int num_of_putted_corners[3];
		int num_of_putted_corners_inner[3];
		int num_of_putted_corners_two_sides[3];
		int num_of_putted_remainings[3];
		for (int i = 0; i < SIZE; i++)for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == EMPTY)continue;//不考慮是EMPTY的情況

			if (Point(i, j) == Corners[0] || Point(i, j) == Corners[1] || Point(i, j) == Corners[2] || Point(i, j) == Corners[3])
				num_of_putted_corners[board[i][j]]++;
			else if (Point(i, j) == Corners_Inner[0] || Point(i, j) == Corners_Inner[1] || Point(i, j) == Corners_Inner[2] || Point(i, j) == Corners_Inner[3])
				num_of_putted_corners_inner[board[i][j]]++;
			else if (
				Point(i, j) == Corners_two_sides[0] || Point(i, j) == Corners_two_sides[1] ||
				Point(i, j) == Corners_two_sides[2] || Point(i, j) == Corners_two_sides[3] ||
				Point(i, j) == Corners_two_sides[4] || Point(i, j) == Corners_two_sides[5] ||
				Point(i, j) == Corners_two_sides[6] || Point(i, j) == Corners_two_sides[7])
				num_of_putted_corners_two_sides[board[i][j]]++;
			else num_of_putted_remainings[board[i][j]] ++;
			
		}
		
		
		if (cur_player == BLACK) {//分數越高代表對BLACK越好//這一行是針對BLACK的valid_to_be_put部分而已
			score += num_of_to_be_put_corners * 100 + num_of_to_be_put_corners_two_sides * 10 - num_of_to_be_put_corners_inner * 15 + num_of_to_be_put_remainings;
		}
		else if (cur_player == WHITE) {//分數越低代表對WHITE越好//這一行是針對WHITE的valid_to_be_put部分而已
			score -= num_of_to_be_put_corners * 100 + num_of_to_be_put_corners_two_sides * 10 - num_of_to_be_put_corners_inner * 15 + num_of_to_be_put_remainings;
		}
		//以下兩行黑白都適用，因為是已經放在盤面上的黑色、白色棋子的分數的計算。如果我這局要下的是白色，那麼上面的if-elseif我只會看到WHITE那行(score為負)。
		//但是除了考慮valid_to_be_put以外，我還要考慮我現在場上的白色棋子的分布狀態是否很好?(佔據了角點?或者角點兩邊?) 而黑色棋子的分布狀態是否對我很不利(佔據了角點?或兩邊?)
		//所以這一部分是黑白都要考慮。
		score += num_of_putted_corners[BLACK] * 150 - num_of_putted_corners_inner[BLACK] * 20 + num_of_putted_corners_two_sides[BLACK] * 15 + num_of_putted_remainings[BLACK] * 3;
		score -= num_of_putted_corners[WHITE] * 150 - num_of_putted_corners_inner[WHITE] * 20 + num_of_putted_corners_two_sides[WHITE] * 15 + num_of_putted_remainings[WHITE] * 3;

		return score;
	}

	if (this->cur_player == BLACK/*MAX*/) {
		//max of(a belongs to Action(cur state), MINIMAX(result of (cur state, a)))

		//針對現有的盤面，去依序放上有所valid的格子，得到所有新的盤面。把所有新的盤面取MINIMAX，找最大的MINIMAX值回傳
		int cur_MAX = INT_MIN;
		for (int i = 0; i < next_valid_spots.size(); i++) {
			OthelloBoard temp = *this;
			temp.put_disc(next_valid_spots[i]);
			int tmp = temp.MiniMax(cur_depth + 1, terminal_depth);

			if (tmp > cur_MAX)cur_MAX = tmp;
		}
		return cur_MAX;
	}
	else if (this->cur_player == WHITE) {
		//min of(a belongs to Action(cur state), MINIMAX(result of (cur state, a)))

		int cur_min = INT_MAX;
		for (int i = 0; i < next_valid_spots.size(); i++) {
			OthelloBoard temp = *this;
			temp.put_disc(next_valid_spots[i]);
			int tmp = temp.MiniMax(cur_depth + 1, terminal_depth);
			
			if (tmp < cur_min)cur_min = tmp;
		}
		return cur_min;
	}

}
