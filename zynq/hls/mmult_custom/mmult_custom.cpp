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

// #pragma HLS bind_storage variable = input type = RAM_T2P
// #pragma HLS bind_storage variable = weight1 type = RAM_T2P
// #pragma HLS bind_storage variable = weight2 type = RAM_T2P
// #pragma HLS bind_storage variable = offset type = RAM_T2P
// #pragma HLS bind_storage variable = out_buf type = RAM_T2P

// #pragma HLS array_partition variable = input block factor = 4 dim = 2
// #pragma HLS array_partition variable = weight1 block factor = 4 dim = 2
// #pragma HLS array_partition variable = weight2 block factor = 4 dim = 2

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
        for (int j = 0; j < HIDDEN; j += W2_WIDTH_RATIO) {
            axi_T packet = pop_stream(in_stream);
            for (int w = 0; w < W2_WIDTH_RATIO; w++) {
                weight2[i][j + w] = packet.w2[w];
            }
        }
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

    FORWARD_COMPUTE:
        for (int i = 0; i < TILING; i++) {
            w1_T hidden[HIDDEN] = {0};

        COMPUTE_HIDDEN:
            for (int j = 0; j < HIDDEN; j++) {
#pragma HLS PIPELINE II = 1
                // Perform the dot product
                w1_T sum = 0;
            H_INNER:
                for (int k = 0; k < FEAT; k++) {
                    sum += input[i][k] * weight1[j][k];
                }
                hidden[j] = sum * (sum > 0);
            }

        COMPUTE_OUTPUT:
            for (int j = 0; j < CLASSES; j++) {
#pragma HLS PIPELINE II = 1
                // Perform the dot product
                out_buf[i][j] = offset[j];
            O_INNER:
                for (int k = 0; k < HIDDEN; k++) {
                    out_buf[i][j] += hidden[k] * weight2[j][k];
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
    volatile ap_uint<sizeof(axi_T)> strb = e.strb;
    volatile ap_uint<sizeof(axi_T)> keep = e.keep;
    volatile ap_uint<AXI_U> user = e.user;
    volatile ap_uint<1> last = e.last;
    volatile ap_uint<AXI_TI> id = e.id;
    volatile ap_uint<AXI_TD> dest = e.dest;

    return ret;
}

void push_stream(hls::stream<AXI_VAL> &is, axi_T const &v, bool last = false)
{
#pragma HLS INLINE

    AXI_VAL e;
    *(axi_T *)(&e.data) = v;
    e.strb = -1;
    e.keep = -1;
    e.user = 0;
    e.last = last ? 1 : 0;
    e.id = 0;
    e.dest = 0;

    is.write(e);
}
