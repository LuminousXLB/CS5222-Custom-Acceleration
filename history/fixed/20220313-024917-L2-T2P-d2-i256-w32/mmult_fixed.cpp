#include <stdio.h>
#include <stdlib.h>

#include "mmult.h"

#define OUT_MASK ((1ULL << OUT_WIDTH) - 1)
#define W_MASK ((1ULL << W_WIDTH) - 1)
#define IN_MASK ((1ULL << IN_WIDTH) - 1)

// --------------------------------------------------------------------
// function to be accelerated in HW wrapped with AXI4-Stream interface
void mmult_hw(hls::stream<AXI_VAL> &in_stream, hls::stream<AXI_VAL> &out_stream)
{
#pragma HLS INTERFACE s_axilite port = return bundle = CONTROL_BUS
#pragma HLS INTERFACE axis port = in_stream
#pragma HLS INTERFACE axis port = out_stream

    // Assertions (to avoid out of array bound writes)
    assert(BATCH % TILING == 0);
    assert(FEAT % W_WIDTH_RATIO == 0);
    assert(FEAT % IN_WIDTH_RATIO == 0);
    assert((BATCH * CLASSES) % OUT_WIDTH_RATIO == 0);

    // Hardware memory buffers
    out_T offset_buf[CLASSES];
    w_T weight_buf[CLASSES][FEAT];
    in_T in_buf[TILING][FEAT];
    out_T out_buf[TILING][CLASSES];

#pragma HLS bind_storage variable = weight_buf type = RAM_T2P
#pragma HLS bind_storage variable = in_buf type = RAM_T2P

#pragma HLS array_partition variable = weight_buf block factor = 32 dim = 2
#pragma HLS array_partition variable = in_buf block factor = 256 dim = 2

    // Input and output AXI stream indices
    int is_idx = 0;
    int os_idx = 0;

// Stream in offset vector
// CSE548 TODO
LOAD_OFFSET:
    for (int i = 0; i < CLASSES; i += OUT_WIDTH_RATIO) {
        AXI_VAL tmp;
        in_stream.read(tmp);
        axi_T packet = pop_stream(tmp);

        for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
            offset_buf[i + w] = (packet >> (w * OUT_WIDTH)) & OUT_MASK;
        };
    }

// Stream in weight matrix
// CSE548 TODO
LOAD_WEIGHT:
    for (int i = 0; i < CLASSES; i++) {
        for (int j = 0; j < FEAT; j += W_WIDTH_RATIO) {
            AXI_VAL tmp;
            in_stream.read(tmp);
            axi_T packet = pop_stream(tmp);

            for (int w = 0; w < W_WIDTH_RATIO; w++) {
                weight_buf[i][j + w] = (packet >> (w * W_WIDTH)) & W_MASK;
            };
        }
    }

// Iterate over tiles
LT:
    for (int t = 0; t < BATCH; t += TILING) {

    // Stream in input tile
    // CSE548 TODO
    LOAD_INPUT:
        for (int i = 0; i < TILING; i++) {
            for (int j = 0; j < FEAT; j += IN_WIDTH_RATIO) {
                AXI_VAL tmp;
                in_stream.read(tmp);
                axi_T packet = pop_stream(tmp);

                for (int w = 0; w < IN_WIDTH_RATIO; w++) {
                    in_buf[i][j + w] = (packet >> (w * IN_WIDTH)) & IN_MASK;
                };
            }
        }

    // Perform matrix multiplication
    L1:
        for (int i = 0; i < TILING; i++) {
        // Iterate over output classes
        L2:
            for (int j = 0; j < CLASSES; j++) {
#pragma HLS PIPELINE II = 1
                // Perform the dot product
                out_T tmp = offset_buf[j];
            L3:
                for (int k = 0; k < FEAT; k++) {
                    out_T mult = in_buf[i][k] * weight_buf[j][k];
                    tmp += mult;
                }
                out_buf[i][j] = tmp;
            }
        }

    // Stream out output matrix
    // CSE548 TODO
    STORE_OUTPUT:
        for (int i = 0; i < TILING; i++) {
            for (int j = 0; j < CLASSES; j += OUT_WIDTH_RATIO) {
                axi_T packet = 0;
                for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
                    out_bit_T bits = *((out_bit_T *)&out_buf[i][j + w]);
                    packet |= (bits & OUT_MASK) << (w * OUT_WIDTH);
                }
                os_idx++;
                out_stream.write(push_stream(packet, os_idx == (OS_SIZE)));
            }
        }
    }
}

// --------------------------------------------------------
// functions to insert and extract elements from an axi stream
// includes conversion to correct data type
axi_T pop_stream(AXI_VAL const &e)
{
#pragma HLS INLINE

    axi_T ret = e.data;

    volatile ap_uint<sizeof(axi_T)> strb = e.strb;
    volatile ap_uint<sizeof(axi_T)> keep = e.keep;
    volatile ap_uint<AXI_U> user = e.user;
    volatile ap_uint<1> last = e.last;
    volatile ap_uint<AXI_TI> id = e.id;
    volatile ap_uint<AXI_TD> dest = e.dest;

    return ret;
}

AXI_VAL push_stream(axi_T const &v, bool last = false)
{
#pragma HLS INLINE

    AXI_VAL e;

    e.data = v;
    e.strb = (1 << sizeof(axi_T)) - 1;
    e.keep = (1 << sizeof(axi_T)) - 1;
    e.user = 0;
    e.last = last ? 1 : 0;
    e.id = 0;
    e.dest = 0;
    return e;
}
