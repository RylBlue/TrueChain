#include "Keccak.h"

//Info from: https://keccak.team/keccak_specs_summary.html

#define VAR_B 1600
#define VAR_W 64
//W = B/25
#define VAR_L 6
//2^L = W
#define VAR_N 24
//N = 12 + 2*L

//w is the length of each "lane" in bits
//This entire file assumes that w is 64


struct keccak256_block{
    uint64_t block[5][5];
};

keccak256_block keccak_R = {
    {
        {0, 18, 41, 3, 36}, //X=0
        {1, 2, 45, 10, 44}, //X=1
        {62, 61, 15, 43, 6}, //X=2
        {28, 56, 21, 25, 55}, //X=3
        {27, 14, 8, 39, 20} //X=4
    }
};
uint64_t keccak_RC[VAR_N] = {
    0x0000000000000001,
    0x0000000000008082,
    0x800000000000808A,
    0x8000000080008000,
    0x000000000000808B,
    0x0000000080000001,
    0x8000000080008081,
    0x8000000000008009,
    0x000000000000008A,
    0x0000000000000088,
    0x0000000080008009,
    0x000000008000000A,
    0x000000008000808B,
    0x800000000000008B,
    0x8000000000008089,
    0x8000000000008003,
    0x8000000000008002,
    0x8000000000000080,
    0x000000000000800A,
    0x800000008000000A,
    0x8000000080008081,
    0x8000000000008080,
    0x0000000080000001,
    0x8000000080008008
};

inline uint64_t left_rotate(uint64_t a, uint32_t count){
    count = count%64;
    return (a<<count) | (a >> (64-count));
}

inline uint64_t right_rotate(uint64_t a, uint32_t count){
    count = count%64;
    return (a>>count) | (a << (64-count));
}

inline void keccakf_round(keccak256_block& A, uint64_t RC){
    uint64_t C[5];
    uint64_t D[5];
    keccak256_block B;

    for(uint32_t x = 0; x<5; ++x){
        C[x] = A.block[x][0] ^ A.block[x][1] ^ A.block[x][2] ^ A.block[x][3] ^ A.block[x][4];
    }
    for(uint32_t x = 5; x<10; ++x){
        D[x-5] = C[(x-1)%5] ^ left_rotate(C[(x+1)%5], 1);
    }
    for(uint32_t x = 0; x<5; ++x){
        for(uint32_t y = 0; y<5; ++y){
            B.block[y][(2*x + 3*y)%5] = left_rotate(A.block[x][y],keccak_R.block[x][y] );
        }
    }
    for(uint32_t x = 0; x<5; ++x){
        for(uint32_t y = 0; y<5; ++y){
            A.block[x][y] = B.block[x][y] ^ (~(B.block[(x+1)%5][y]) & B.block[(x+2)%5][y]);
        }
    }
    A.block[0][0] ^= RC;
}

inline void keccakf(keccak256_block& A){
    for(uint32_t i = 0; i<VAR_N; ++i){
        keccakf_round(A, keccak_RC[i]);
    }
}

//capcity + bitrate is assumed to be equal to VAR_B
//Extra bit count must be between 0 and 7
//I slightly modified this function to accept a "partial hash" as well
//This modifies the base state of S[x][y] which subsequently alters the overall hash
uint256_t keccak_complete(uint32_t bitrate, uint32_t capacity, uint8_t extra_bit_count, uint8_t extra_bits, const uint8_t* main_data, uint32_t byte_count, const uint256_t& partial_hash){
    uint8_t d = (1 << (7 - (extra_bit_count%8))) | ((0xFF >> (8-(extra_bit_count%8))) & extra_bits);
    //P = concat(main_data, d, padding)
    //last byte of P is XORed with 0x80
    //padding is to make P's byte size a multiple of keccak256_block's byte size (8*5*5) = (200)


    keccak256_block S;
    for(uint32_t y = 0; y<5; ++y){
        for(uint32_t x = 0; x<5; ++x){
            S.block[x][y] = 0;
        }
    }

    //Non-standard partial_hash shenanigans (NOT PART OF THE ORIGNAL SPEC)
    for(uint32_t i = 0; i<8; ++i){
        S.block[0][0] |= ((uint64_t)partial_hash.I[0+i]) << (8*i);
        S.block[4][0] |= ((uint64_t)partial_hash.I[8+i]) << (8*i);
        S.block[0][4] |= ((uint64_t)partial_hash.I[16+i]) << (8*i);
        S.block[4][4] |= ((uint64_t)partial_hash.I[24+i]) << (8*i);
    }
    //

    uint32_t block_count = ((byte_count + (uint64_t)1)/200) + ((byte_count + (uint64_t)1)%200 ? 1 : 0);
    
    for(uint32_t i = 0; i < (block_count)*VAR_B; i+=bitrate){
        bool to_continue = true;
        for(uint32_t y = 0; y<5 && to_continue; ++y){
            for(uint32_t x = 0; x<5 && to_continue; ++x){
                if(x + 5*y >= bitrate/VAR_W){
                    to_continue = false;
                    continue;
                }
                uint32_t test = (x + 5*y) * 8 + i/8;
                uint64_t Pi = 0;
                for(uint32_t bc = 0; bc<8; ++bc){
                    if(test+bc<byte_count){
                        Pi |= ((uint64_t)main_data[test+bc])<<(bc*8);
                    }
                    else if(test+bc == byte_count){
                        Pi |= ((uint64_t)d)<<(bc*8);
                    }
                }
                if(i + bitrate >= block_count*VAR_B && x + 5*y == 24){
                        Pi ^= 0x8000000000000000; //Final byte of P is XORed with 0x80
                }

                S.block[x][y] ^= Pi;

            }
        }
        keccakf(S);
    }

    //Now to generate the 256 bit output hash
    uint256_t to_return;
    uint32_t x = 0, y = 0;
    for(uint32_t i = 0; i<4; ++i){
        uint64_t temp;
        if(x + 5*y >= bitrate/VAR_W){
            x = 0;
            y = 0;
            keccakf(S);
            continue;
        }
        temp = S.block[x][y];
        for(uint32_t bc = 0; bc<8; ++bc){
            to_return.I[bc + 8*i]=(temp>>(bc*8))&0xFF;
        }
        if(x == 4){
            if(y == 4){
                x = 0;
                y = 0;
                keccakf(S);
                continue;
            }
            ++y;
            x = 0;
            continue;
        }
        ++x;
    }
    return to_return;
}

uint256_t custom_keccak256(uint32_t byte_length, const uint8_t* data, const uint256_t& partial_hash){
    //x,y,z form a 3D cube of data points
    //x and y are bounded from 0 to 4
    //z is bounded from 0 to VAR_W - 1 = (VAR_B)/(5 * 5) - 1
    //The "cube" is indexed with <0,0,z> representing the center lane
    //adding one to the x or y coordinates moves the lane to the right or up
    //The lane wraps around to the left side or bottom side on (2=>3)

    //I.e. 3 neighbors 4, 4 neighbors 3 and 0, 0 neighbors 4 and 1, 1 neighbors 2 and 0, and 2 neighbors 1

    //rate = VAR_B ensures that all bits are processed in a single chunk
    //rate must be less than or equal to VAR_B, greater than or equal to VAR_W, and a multiple of VAR_W (also a multiple of 8)
    //capacity must equal VAR_B - rate
    return keccak_complete(1088, VAR_B - 1088, 0, 0x00, data, byte_length, partial_hash);
}