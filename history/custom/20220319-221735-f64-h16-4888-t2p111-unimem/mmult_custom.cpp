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
    assert((BATCH * CLASSES) % OUT_WIDTH_RATIO == 0);

    // Hardware memory buffers
    in_T input[BATCH][FEAT];
    w1_T weight1[HIDDEN][FEAT];
    w2_T weight2[CLASSES][HIDDEN];
    out_T offset[CLASSES];
    out_T out_buf[TILING][CLASSES];

#pragma HLS bind_storage variable = input type = RAM_T2P
#pragma HLS bind_storage variable = weight1 type = RAM_T2P
#pragma HLS bind_storage variable = weight2 type = RAM_T2P
#pragma HLS bind_storage variable = offset type = RAM_T2P
#pragma HLS bind_storage variable = out_buf type = RAM_T2P

#pragma HLS array_partition variable = input block factor = 2 dim = 2
#pragma HLS array_partition variable = weight1 block factor = 2 dim = 2
#pragma HLS array_partition variable = weight2 block factor = 2 dim = 2

// Stream in offset vector
LOAD_OFFSET:
    axi_T packet = pop_stream(in_stream);
    for (int i = 0; i < CLASSES; i++) {
#pragma HLS PIPELINE II = 1
        offset[i] = packet.o[i];
    }

// Stream in weight matrix
LOAD_WEIGHT1:
    for (int i = 0; i < HIDDEN; i++) {
#pragma HLS PIPELINE II = 1
        for (int j = 0; j < FEAT; j += W1_WIDTH_RATIO) {
            axi_T packet = pop_stream(in_stream);
#if W1_WIDTH == 4
            for (int w = 0; w < W1_WIDTH_RATIO / 2; w++) {
                weight1[i][j + 2 * w] = packet.w1[w].a0;
                weight1[i][j + 2 * w + 1] = packet.w1[w].a1;
            }
#else
            for (int w = 0; w < W1_WIDTH_RATIO; w++) {
                weight1[i][j + w] = packet.w1[w];
            }
#endif
        }
    }

LOAD_WEIGHT2:
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
                    input[i][j + 2 * w] = packet.i[w].a0;
                    input[i][j + 2 * w + 1] = packet.i[w].a1;
                };
            }
        }

        w1_T hidden[TILING][HIDDEN] = {0};

        // FORWARD_COMPUTE:
    COMPUTE_HIDDEN:
        for (int i = 0; i < TILING; i++) {
        H_INNER1:
            for (int h = 0; h < HIDDEN; h++) {
#pragma HLS PIPELINE II = 1
                // Perform the dot product
                w1_T sum = 0;
            H_INNER2:
                for (int f = 0; f < FEAT; f++) {
                    sum += input[i][f] * weight1[h][f];
                }
                hidden[i][h] = sum * (sum > 0);
            }
        }

    COMPUTE_OUTPUT:
        for (int i = 0; i < TILING; i++) {
        O_INNER1:
            for (int c = 0; c < CLASSES; c++) {
#pragma HLS PIPELINE II = 1
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
            axi_T packet;
            for (int j = 0; j < CLASSES; j++) {
                packet.o[j] = out_buf[i][j];
            }
            push_stream(out_stream, packet, (t + i) == (BATCH - 1));
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

    axi_T ret = *(axi_T *)(&e.data);
    volatile ap_uint<1> last = e.last;

    return ret;
}

void push_stream(hls::stream<AXI_VAL> &is, axi_T const &v, bool last = false)
{
#pragma HLS INLINE

    AXI_VAL e;
    *(axi_T *)(&e.data) = v;
    e.last = last ? 1 : 0;

    is.write(e);
}
