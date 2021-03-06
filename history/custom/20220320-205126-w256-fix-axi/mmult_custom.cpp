#include <cstdint>
#include <stdio.h>
#include <stdlib.h>

#include "mmult.h"

// --------------------------------------------------------------------
// function to be accelerated in HW wrapped with AXI4-Stream interface
void mmult_hw(hls::stream<AXI_VAL> &in_stream, hls::stream<AXI_VAL> &out_stream)
{
#pragma HLS INTERFACE s_axilite port = return bundle = CONTROL_BUS
#pragma HLS INTERFACE axis port = in_stream
#pragma HLS INTERFACE axis port = out_stream

    // Assertions (to avoid out of array bound writes)
    assert(BATCH % TILING == 0);
    assert(FEAT % IN_WIDTH_RATIO == 0);
    assert(FEAT % W1_WIDTH_RATIO == 0);
    assert(FEAT % W2_WIDTH_RATIO == 0);
    // assert((BATCH * CLASSES) % OUT_WIDTH_RATIO == 0);

    // Hardware memory buffers
    in_T input[BATCH][FEAT];
    w1_T weight1[HIDDEN][FEAT];
    w2_T weight2[CLASSES][HIDDEN];
    out_T hidden[TILING][HIDDEN];
#if CLASSES < OUT_WIDTH_RATIO
    out_T offset[CLASSES];
    out_T out_buf[TILING][CLASSES];
#elif OUT_WIDTH_RATIO == 4
    out_T offset[CLASSES + 2];
    out_T out_buf[TILING][CLASSES + 2];
#endif

#pragma HLS bind_storage variable = input type = RAM_T2P
#pragma HLS bind_storage variable = weight1 type = RAM_T2P
#pragma HLS bind_storage variable = weight2 type = RAM_T2P

#pragma HLS array_partition variable = input block factor = 32 dim = 2
#pragma HLS array_partition variable = weight1 block factor = 32 dim = 2
#pragma HLS array_partition variable = weight2 block factor = 2 dim = 2

// Stream in offset vector
LOAD_OFFSET:
#if CLASSES < OUT_WIDTH_RATIO
    axi_T packet = pop_stream(in_stream);
    for (int i = 0; i < CLASSES; i++) {
#pragma HLS PIPELINE II = 1
        offset[i] = packet.o[i];
    }
#else
    for (int i = 0; i < CLASSES; i += OUT_WIDTH_RATIO) {
#pragma HLS PIPELINE II = 1
        axi_T packet = pop_stream(in_stream);
        for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
            offset[i + w] = packet.o[w];
        }
    }
#endif

// Stream in weight matrix
LOAD_WEIGHT1:
    for (int i = 0; i < HIDDEN; i++) {
#pragma HLS PIPELINE II = 1
        for (int j = 0; j < FEAT; j += W1_WIDTH_RATIO) {
            axi_T packet = pop_stream(in_stream);
            for (int w = 0; w < W1_WIDTH_RATIO; w++) {
                weight1[i][j + w] = packet.w1[w];
            }
        }
    }

LOAD_WEIGHT2:
#if W2_WIDTH_RATIO == 2 * HIDDEN
    for (int i = 0; i < CLASSES; i += 2) {
#pragma HLS PIPELINE II = 1
        axi_T packet = pop_stream(in_stream);
        for (int j = 0; j < HIDDEN; j++) {
            weight2[i][j] = packet.w2[j];
        }

        for (int j = 0; j < HIDDEN; j++) {
            weight2[i + 1][j] = packet.w2[j + HIDDEN];
        }
    }
#else
    for (int i = 0; i < CLASSES; i++) {
#pragma HLS PIPELINE II = 1
#if W2_WIDTH_RATIO <= HIDDEN
        for (int j = 0; j < HIDDEN; j += W2_WIDTH_RATIO) {
            axi_T packet = pop_stream(in_stream);
            for (int w = 0; w < W2_WIDTH_RATIO; w++) {
                weight2[i][j + w] = packet.w2[w];
            }
        }
#else
        axi_T packet = pop_stream(in_stream);
        for (int j = 0; j < HIDDEN; j++) {
            weight2[i][j] = packet.w2[j];
        }
#endif
    }
#endif

// Iterate over tiles
LT:
    for (int t = 0; t < BATCH; t += TILING) {

    // Stream in input tile
    LOAD_INPUT:
        for (int i = 0; i < TILING; i++) {
#pragma HLS PIPELINE II = 1
        LOAD:
            for (int j = 0; j < FEAT; j += IN_WIDTH_RATIO) {
                axi_T packet = pop_stream(in_stream);
                for (int w = 0; w < IN_WIDTH_RATIO / 2; w++) {
                    input[i][j + 2 * w] = packet.i[w].a1;
                    input[i][j + 2 * w + 1] = packet.i[w].a0;
                };
            }
        }

    COMPUTE_HIDDEN:
        for (int i = 0; i < TILING; i++) {
        H_INNER1:
            for (int h = 0; h < HIDDEN; h++) {
#pragma HLS PIPELINE II = 1
                // Perform the dot product
                out_T sum = 0;
            H_INNER2:
                for (int f = 0; f < FEAT; f++) {
                    sum += input[i][f] * weight1[h][f];
                }
                hidden[i][h] = sum * (sum > 0);
            }
        }

    COMPUTE_OUTPUT:
        for (int i = 0; i < TILING; i++) {
#pragma HLS PIPELINE II = 1
        O_INNER1:
            for (int c = 0; c < CLASSES; c++) {
                // Perform the dot product
                out_buf[i][c] = offset[c];
            O_INNER2:
                for (int h = 0; h < HIDDEN; h++) {
                    out_buf[i][c] += hidden[i][h] * weight2[c][h];
                }
            }
        }

    // Stream out output matrix
    STORE_OUTPUT:
        for (int i = 0; i < TILING; i++) {
#pragma HLS PIPELINE II = 1

#if CLASSES < OUT_WIDTH_RATIO
            axi_T packet;
            for (int j = 0; j < CLASSES; j++) {
                packet.o[j] = out_buf[i][j];
            }

#define LAST ((t + i) == (BATCH - 1))
            if (LAST) {
                printf("FPGA PUSHING %d (last=%d)\n", t + i, LAST);
            }
            push_stream(out_stream, packet, LAST);
#undef LAST
#else
            for (int j = 0; j < CLASSES; j += OUT_WIDTH_RATIO) {
                axi_T packet;
                for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
                    packet.o[w] = out_buf[i][j + w];
                }
#define LAST (((t + i) == (BATCH - 1)) && (j == (OUT_WIDTH_RATIO * (CLASSES / OUT_WIDTH_RATIO))))
                if (LAST) {
                    printf("FPGA PUSHING %d (last=%d)\n", t + i, LAST);
                }
                push_stream(out_stream, packet, LAST);
#undef LAST
            }

#endif
        }
    }
}

// --------------------------------------------------------
// functions to insert and extract elements from an axi stream
// includes conversion to correct data type
axi_T pop_stream(hls::stream<AXI_VAL> &is)
{
#pragma HLS INLINE

    AXI_VAL e;
    is.read(e);

    volatile ap_uint<sizeof(axi_T)> strb = e.strb;
    volatile ap_uint<sizeof(axi_T)> keep = e.keep;
    volatile ap_uint<AXI_U> user = e.user;
    volatile ap_uint<1> last = e.last;
    volatile ap_uint<AXI_TI> id = e.id;
    volatile ap_uint<AXI_TD> dest = e.dest;

    axi_T packet;

    packet._packet[0] = e.data.range((0 + 1) * 64 - 1, 0 * 64);
    packet._packet[1] = e.data.range((1 + 1) * 64 - 1, 1 * 64);
    packet._packet[2] = e.data.range((2 + 1) * 64 - 1, 2 * 64);
    packet._packet[3] = e.data.range((3 + 1) * 64 - 1, 3 * 64);

    return packet;
}

void push_stream(hls::stream<AXI_VAL> &os, axi_T const &v, bool last = false)
{
#pragma HLS INLINE

    AXI_VAL e;

    e.data.range((0 + 1) * 64 - 1, 0 * 64) = v._packet[0];
    e.data.range((1 + 1) * 64 - 1, 1 * 64) = v._packet[1];
    e.data.range((2 + 1) * 64 - 1, 2 * 64) = v._packet[2];
    e.data.range((3 + 1) * 64 - 1, 3 * 64) = v._packet[3];

    e.strb = -1;
    e.keep = -1;
    e.user = 0;
    e.last = last ? 1 : 0;
    e.id = 0;
    e.dest = 0;

    os.write(e);
}
