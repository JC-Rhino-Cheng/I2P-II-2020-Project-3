#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <climits>

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
const int Latter_Quarter_Threshold = SIZE * SIZE * 3 / 4;
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

const std::array<Point, 4> Corners{ {
		Point(0, 0), Point(0, SIZE - 1),
		Point(SIZE - 1, 0),Point(SIZE - 1, SIZE - 1)
		//���W�B�k�W
		//���U�B�k�U
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


class OthelloBoard {
public:
	enum SPOT_STATE {
		EMPTY = 0,
		BLACK = 1,
		WHITE = 2
	};
	enum JiaQuan_Val {//�H�Ugood�Bbad�Ҭ��w��¦�Ө�
		Good_Corner = 1500,
		Bad_Corner = -2000,
		Bad_Corner_Inner = -500,
		Four_Sides = 75,
		Bad_Four_Sides = -100,
		Ordinary_Disc = 50,
		Enemy_Ordinary_Disc = -60,

		Mobility = 67,
		Flipping = 85
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
	//�z�L�ǤJ�ثe��player�s���A����U�@��player�s��
	int get_next_player(int player) const {
		return 3 - player;
	}
	//�ˬd�O�_�W�X�ù����Ľd��
	bool is_spot_on_board(Point p) const {
		return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
	}
	//��Y�@�ӯS�w�y�ЩҨ㦳��disc�s���Ǧ^�C�i�઺��: 0-EMPTY�B1-BLACK�B2-WHITE
	int get_disc(Point p) const {
		return board[p.x][p.y];
	}
	//��Y�@�ӯS�w�y�Эקאּ�S�w��disc�s���C�i�઺��: 0-EMPTY�B1-BLACK�B2-WHITE
	//�����Ψ�A�]���᭱�|�A�~�]��put_disc����
	void set_disc(Point p, int disc) {
		board[p.x][p.y] = disc;
	}
	//��J��m�y�ЩM�Q�n��disc�s���A�ˬd�O�_�T��b���Ӧ�m���۷Q�n��disc
	bool is_disc_at(Point p, int disc) const {
		if (!is_spot_on_board(p))
			return false;
		if (get_disc(p) != disc)
			return false;
		return true;
	}
	//�b�Q�n��@�ӴѤl�U�U�h�@�Ӧ�m���e�A�Ұ����ˬd�C���F�H�U�X��Ʊ�:
	//(1)���ˬd���I�O�_�w�g���i�U�U�h
	//(2)���K�Ӥ�V�ˬd�A�ݬݬO�_���P���C�⪺�F��b�����ݦ��]���A�H�Ѧ��Ī��U�k�C
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
	//�U�U�@�ӴѤl����A���K�Ӥ��i���ˬd�A�ݬݦp�G�o�Ӥ��T��O�i�H�Qflip���ܡA����list�O�U�ӻݭnflip���y�СA����@�Ӥ���ˬd��Τ@�B�z�C����Adisc_count��������T�@�ֽվ�
	//�����ϥΡA�]���|�~�]��put_disc�ϥ�
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
	int get_how_many_can_flip(Point center) {
		std::vector<Point>list;
		for (Point dir : directions) {
			// Move along the direction while testing.
			Point p = center + dir;
			if (!is_disc_at(p, get_next_player(cur_player)))
				continue;

			list.push_back(p);
			p = p + dir;
			while (is_spot_on_board(p) && get_disc(p) != EMPTY) {
				if (is_disc_at(p, cur_player)) break;
				list.push_back(p);
				p = p + dir;
			}
		}

		int JiaQuan_Flipping = 0;
		for (int i = 0; i < list.size(); i++) {
			if (list[i] == Corners[0] || list[i] == Corners[1] || list[i] == Corners[2] || list[i] == Corners[3])JiaQuan_Flipping += 15;
			else if (list[i].x == 0 || list[i].x == SIZE - 1 || list[i].y == 0 || list[i].y == SIZE - 1)JiaQuan_Flipping += 10;
			else JiaQuan_Flipping += 3;
		}

		return JiaQuan_Flipping;
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
	//�q�{����2D�L���غcBoard
	OthelloBoard(std::array<std::array<int, SIZE>, SIZE> brd, int player_idx) : done(false), winner(-1), cur_player(player_idx) {
		disc_count[EMPTY] = disc_count[BLACK] = disc_count[WHITE] = 0;

		for (int i = 0; i < SIZE; i++)for (int j = 0; j < SIZE; j++) {
			board[i][j] = brd[i][j];

			disc_count[board[i][j]]++;//�O����Ѥl�A�N�⨺�ӴѤl��count��++�C
		}

	}
	//��L���٭��̪�l���ѽL
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
	//�b�@�Ӧ��Ħ�m�Q�U�U�h����A��s�Ҧ��s��valid spots�C
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
	//�p�G�U����m�O�����A����Ĺ�a�O�ִN�X�l�F�C�_�h���ܴN�O��svalid spots�C�Ӧp�G�{�b��valid spots�O0�ӡA�ӦA�U�@���٬O0�Ӫ��ܡA�N���Ӫ��a���w�g�S�����i�H���F�A�N���۹C���w�g�����C
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
	int n_valid_spots = next_valid_spots.size();//�`�@���o��h�i�H�U���L��

	OthelloBoard curState(brd, player);//����ثemain.exe�Ҥw�g���Ѫ��L���غc�X�ӡC�D�ظ�����ڲ{�b�n�U���Oplayer(ex: �o�̥H�ΥH�U���Ҥl���ΧڷQ�n�U�զ�ӻ���)

	for (int terminal_depth = 0; terminal_depth < 6; terminal_depth++) {
		std::vector<int>State_Value;//�����C��valid�U�k���n�a�{��
		State_Value.resize(n_valid_spots, 0);

		for (int i = 0; i < n_valid_spots; i++) {
			Point p = next_valid_spots[i];

			OthelloBoard temp = curState;
			temp.put_disc(p);//�ڤw�g��զ�Ѥl�U�U�h�F�C�ӥثetemp�̪�cur_player�|�۰ʧ令�¦�C
			State_Value[i] = temp.MiniMax(0, terminal_depth);
		}
		//�}�l��X�̨Ϊ���k
		//�]����\�L������X�A�ҥH�ڥu�n�C���@�Ӥ���n����k�A�N��X�@���A�Y�i
		//�O�o�Ϥ��{�b��player�쩳�OBLACK�٬OWHITE
		if (player == curState.BLACK) {
			int now_MAX = INT_MIN;
			for (int i = 0; i < n_valid_spots; i++)if (State_Value[i] > now_MAX) {
				now_MAX = State_Value[i];

				fout << next_valid_spots[i].x << " " << next_valid_spots[i].y << std::endl;
				fout.flush();
			}
		}
		else if (player == curState.WHITE) {
			//�]���ڤU���O�զ⪺�A�ҥHState_Value�̭��x�s���O�ڨC�ӤU�k��u�¦�v�������h�V�A�ҥH�ڥu�n����¦�������V��(�ƭȳ̤p��)�A�N�O��ڥզ�̦n��
			int now_min = INT_MAX;
			for (int i = 0; i < n_valid_spots; i++)if (State_Value[i] < now_min) {
				now_min = State_Value[i];

				fout << next_valid_spots[i].x << " " << next_valid_spots[i].y << std::endl;
				fout.flush();
			}
		}
	}















}




int OthelloBoard::MiniMax(int cur_depth, int terminal_depth) {
	//���p��ثe��valid����l������
	//���ެOif(cur_depth == terminal_depth)�٬O�᭱��ӡA���|�Ψ�
	next_valid_spots = get_valid_spots();

	if (cur_depth == terminal_depth) {
		int score = 0;
		if (cur_player == BLACK) {
			for (int i = 0; i < SIZE; i++)for (int j = 0; j < SIZE; j++) {
				Point p(i, j);
				if (p == Corners[0] || p == Corners[1] || p == Corners[2] || p == Corners[3]) {
					if (board[p.x][p.y] == BLACK)score += Good_Corner;
					else if (board[p.x][p.y] == WHITE)score += Bad_Corner;
				}
				else if (p.x == 0 || p.x == SIZE - 1) {
					if (board[p.x][0] == BLACK && board[p.x][SIZE - 2] == BLACK)score += Four_Sides;
					else if (board[p.x][0] == WHITE && board[p.x][SIZE - 2] == WHITE)score += Bad_Four_Sides;
				}
				else if (p.y == 0 || p.y == SIZE - 1) {
					if (board[0][p.y] == BLACK && board[SIZE - 2][p.y] == BLACK)score += Four_Sides;
					else if (board[0][p.y] == WHITE && board[SIZE - 2][p.y] == WHITE)score += Bad_Four_Sides;
				}

				if (disc_count[WHITE] + disc_count[BLACK] >= Latter_Quarter_Threshold) {
					if (board[p.x][p.y] == BLACK)score += Ordinary_Disc;
					else if (board[p.x][p.y] == WHITE)score += Enemy_Ordinary_Disc;
				}
			}
		}
		else if (cur_player == WHITE) {
			for (int i = 0; i < SIZE; i++)for (int j = 0; j < SIZE; j++) {
				Point p(i, j);
				if (p == Corners[0] || p == Corners[1] || p == Corners[2] || p == Corners[3]) {
					if (board[p.x][p.y] == WHITE)score += Good_Corner;
					else if (board[p.x][p.y] == BLACK)score += Bad_Corner;
				}
				else if (p.x == 0 || p.x == SIZE - 1) {
					if (board[p.x][0] == WHITE && board[p.x][SIZE - 2] == WHITE)score += Four_Sides;
					else if (board[p.x][0] == BLACK && board[p.x][SIZE - 2] == BLACK)score += Bad_Four_Sides;
				}
				else if (p.y == 0 || p.y == SIZE - 1) {
					if (board[0][p.y] == WHITE && board[SIZE - 2][p.y] == WHITE)score += Four_Sides;
					else if (board[0][p.y] == BLACK && board[SIZE - 2][p.y] == BLACK)score += Bad_Four_Sides;
				}

				if (disc_count[WHITE] + disc_count[BLACK] >= Latter_Quarter_Threshold) {
					if (board[p.x][p.y] == WHITE)score += Ordinary_Disc;
					else if (board[p.x][p.y] == BLACK)score += Enemy_Ordinary_Disc;
				}
			}
		}

		if (disc_count[WHITE] + disc_count[BLACK] < Latter_Quarter_Threshold) score += next_valid_spots.size()*Mobility;
		if (disc_count[WHITE] + disc_count[BLACK] >= Latter_Quarter_Threshold) {
			int total_how_many_can_flip = 0;
			for (int i = 0; i < next_valid_spots.size(); i++)total_how_many_can_flip += get_how_many_can_flip(next_valid_spots[i]);
			score += total_how_many_can_flip * Flipping;
		}
		//�`�Nreturn���ɭԡA�쩳�O�֦b�U��?�¦�?�զ�?
		return (cur_player == BLACK) ? score : -score;
	}

	if (this->cur_player == BLACK/*MAX*/) {
		//max of(a belongs to Action(cur state), MINIMAX(result of (cur state, a)))

		//�w��{�����L���A�h�̧ǩ�W����valid����l�A�o��Ҧ��s���L���C��Ҧ��s���L����MINIMAX�A��̤j��MINIMAX�Ȧ^��
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
