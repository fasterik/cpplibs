#include <cstdio>
#include <bit>
#include <random>
#include <immintrin.h>
#include "random.h"

using namespace std;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;

enum {in_progress, draw, x_wins, o_wins};

struct Board {
    u16 occ[2];
    u8 stm;
};

u8 result_table[512][512];

int get_result_slow(Board &board) {
    u16 x = board.occ[0];
    u16 o = board.occ[1];
    u16 all = x | o;

    if ((x & 0b000000111) == 0b000000111) return x_wins;
    if ((x & 0b000111000) == 0b000111000) return x_wins;
    if ((x & 0b111000000) == 0b111000000) return x_wins;
    if ((x & 0b100100100) == 0b100100100) return x_wins;
    if ((x & 0b010010010) == 0b010010010) return x_wins;
    if ((x & 0b001001001) == 0b001001001) return x_wins;
    if ((x & 0b100010001) == 0b100010001) return x_wins;
    if ((x & 0b001010100) == 0b001010100) return x_wins;
    if ((o & 0b000000111) == 0b000000111) return o_wins;
    if ((o & 0b000111000) == 0b000111000) return o_wins;
    if ((o & 0b111000000) == 0b111000000) return o_wins;
    if ((o & 0b100100100) == 0b100100100) return o_wins;
    if ((o & 0b010010010) == 0b010010010) return o_wins;
    if ((o & 0b001001001) == 0b001001001) return o_wins;
    if ((o & 0b100010001) == 0b100010001) return o_wins;
    if ((o & 0b001010100) == 0b001010100) return o_wins;

    return popcount(all) > 8 ? draw : in_progress;
}

int get_result(Board &board) {
    return result_table[board.occ[0]][board.occ[1]];
}

void init_tables() {
    for (u16 i = 0; i < 512; i++) {
        for (u16 j = 0; j < 512; j++) {
            Board board = {};
            board.occ[0] = i;
            board.occ[1] = j;

            result_table[i][j] = get_result_slow(board);
        }
    }
}

u16 get_empty(Board &board) {
    return ~(board.occ[0] | board.occ[1]) & 0x1ff;
}

void do_move(Board &board, int sq) {
    board.occ[board.stm] ^= 1 << sq;
    board.stm ^= 1;
}

int get_nth_set_bit(u16 bb, int n) {
    return _tzcnt_u64(_pdep_u64(1ull << n, bb));
}

template<typename T>
int get_random_move(Board &board, T random) {
    u16 moves = get_empty(board);
    int count = popcount(moves);
    int index = random(count);

    return get_nth_set_bit(moves, index);
}

template<typename T>
void play_random_games(const char *name, T random) {
    u64 games = 0;
    auto t1 = chrono::high_resolution_clock::now();

    while (games < 1e8) {
        Board board = {};
        games++;

        while (true) {
            do_move(board, get_random_move(board, random));
            if (get_result(board) != in_progress)
                break;
        }
    }

    auto t2 = chrono::high_resolution_clock::now();
    auto micros = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
    double mgps = (double)games / micros;

    printf("%s: %.1fM games/s\n", name, mgps);
}

int main() {
    init_tables();

    Random random;

    random_device rd;
    mt19937_64 mt(rd());
    ranlux48_base ranlux(rd());

    play_random_games("xoroshiro256++", [&](int count) {
        return (int)random.get_range(count);
    });

    play_random_games("mt19937_64", [&](int count) {
        uniform_int_distribution<int> dist(0, count - 1);
        return dist(mt);
    });

    play_random_games("ranlux48_base", [&](int count) {
        uniform_int_distribution<int> dist(0, count - 1);
        return dist(ranlux);
    });

    return 0;
}
