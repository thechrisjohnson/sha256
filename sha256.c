/* 
 * Copyright 2012 Christopher Johnson
 *
 * This file is part of sha256.
 *
 * sha256 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sha256 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sha256.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <endian.h>
#include <stdio.h>
#include <unistd.h>

#include "sha256.h"

ssize_t CHUNK_SIZE = 64;
ssize_t LENGTH_SIZE = 8;
uint64_t total_length = 0;

int main()
{
    uint32_t h0 = H0;
    uint32_t h1 = H1;
    uint32_t h2 = H2;
    uint32_t h3 = H3;
    uint32_t h4 = H4;
    uint32_t h5 = H5;
    uint32_t h6 = H6;
    uint32_t h7 = H7;

    unsigned char chunk[CHUNK_SIZE];
    bool shouldBreak = false;
    uint32_t before = 0xAAAAAAAA;

    while (true) {
        if (shouldBreak) {
            break;
        }

        shouldBreak = !get_chunk(chunk);

        uint32_t w[64];
        // Initialize the first 16 things
        for (int i = 0; i < 16; i++) {
            w[i] = get_int(chunk, (i * 4));
            printf("WORD[%i]:  0x%x\n", i, w[i]);
        }

        // Now set the rest of the current schedule
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = ROTR(w[i-15], 7) ^ ROTR(w[i-15], 18) ^ (w[i-15]>>3);
            uint32_t s1 = ROTR(w[i-2], 17) ^ ROTR(w[i-2], 19) ^ (w[i-2]>>18);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        // Initialize working variables to current hash value:
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;

        // Compression function main loop:
        for (int i = 0; i < 64; i++) {
            uint32_t s1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + s1 + ch + K[i] + w[i];
            uint32_t s0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = s0 + maj;
 
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }

    //printf("0x%x%x%x%x%x%x%x%x", htobe32(h0), htobe32(h1), htobe32(h2), htobe32(h3), htobe32(h4), htobe32(h5), htobe32(h6), htobe32(h7));
    printf("0x%x%x%x%x%x%x%x%x", h0, h1, h2, h3, h4, h5, h6, h7);
    return 0;
}

// Read out 512 bits. If we're at less than 512, pad the end with a 1 and then 0's
// Result is if we have more data to read
bool get_chunk(unsigned char buffer[]) {
    ssize_t bytes_read = read(STDIN_FILENO, buffer, CHUNK_SIZE);

    // Add the amount we read to the total length
    total_length += bytes_read;
    printf("bytes_read:  %32b\n", bytes_read);
    printf("total_length:  %32b\n", total_length);

    if (bytes_read < CHUNK_SIZE) {
        // We're at the end of the stream, so we need to pad the end
        // Add a 1, and then 0 bits so we have a chunk of 512 bits (64 bytes)
        buffer[bytes_read] = 0x80; 
        for (int i = bytes_read + 1; i < CHUNK_SIZE - LENGTH_SIZE; i++) {
            buffer[i] = 0x00;
        }

        // Now append the big-endian version of the byte
        uint64_t length = total_length * 8;
        printf("length:  %32b\n", length);
        for (int i = 0; i < LENGTH_SIZE; i++) {
            buffer[CHUNK_SIZE - LENGTH_SIZE + i] = length >> 56 - (i*8);
        }

        return false;
    } else {
        return true;
    }
}

uint32_t get_int(unsigned char buffer[], int startingPoint) {
    return ((uint32_t)buffer[startingPoint]) << 24 |
      ((uint32_t)buffer[startingPoint + 1]) << 16 |
      ((uint32_t)buffer[startingPoint + 2]) << 8  |
      ((uint32_t)buffer[startingPoint + 3]);
}
