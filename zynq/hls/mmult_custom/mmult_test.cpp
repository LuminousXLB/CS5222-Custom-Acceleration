#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "mmult.h"

void matrix_multiply_ref(
    in_T inputs[BATCH][FEAT],
    w_T weight1[HIDDEN][FEAT],
    w_T weight2[CLASSES][HIDDEN],
    out_T offsets[CLASSES],
    out_T out[BATCH][CLASSES])
{

    out_T hidden[BATCH][HIDDEN];

    // matrix multiplication of a A*B matrix
    for (int i = 0; i < BATCH; ++i) {
        for (int j = 0; j < HIDDEN; ++j) {
            int16_t sum = 0;
            for (int k = 0; k < FEAT; ++k) {
                sum += inputs[i][k] * weight1[j][k];
            }
            hidden[i][j] = sum > 0 ? sum : 0;
        }
    }

    for (int i = 0; i < BATCH; ++i) {
        for (int j = 0; j < CLASSES; ++j) {
            int16_t sum = offsets[j];
            for (int k = 0; k < HIDDEN; ++k) {
                sum += hidden[i][k] * weight2[j][k];
            }
            out[i][j] = sum;
        }
    }

    return;
}

int main(void)
{
    int i, j, err;

    in_T inputs[BATCH][FEAT];
    w_T weight1[HIDDEN][FEAT];
    w_T weight2[CLASSES][HIDDEN];
    out_T offsets[CLASSES];

    out_T output_sw[BATCH][CLASSES];
    out_T output_hw[BATCH][CLASSES];

    /** Matrix Initiation */
    for (i = 0; i < BATCH; i++) {
        for (j = 0; j < FEAT; j++) {
            inputs[i][j] = (in_T)(rand() % (1ULL << IN_WIDTH));
        }
    }

    for (i = 0; i < HIDDEN; i++) {
        for (j = 0; j < FEAT; j++) {
            weight1[i][j] = (w_T)((rand() % (1ULL << W_WIDTH)) - (1ULL << (W_WIDTH - 1)));
        }
    }

    for (i = 0; i < CLASSES; i++) {
        for (j = 0; j < HIDDEN; j++) {
            weight2[i][j] = (w_T)((rand() % (1ULL << W_WIDTH)) - (1ULL << (W_WIDTH - 1)));
        }
    }

    for (i = 0; i < CLASSES; i++) {
        offsets[i] = (out_T)((rand() % (1ULL << OUT_WIDTH)) - (1ULL << (OUT_WIDTH - 1)));
    }

    /** End of Initiation */

    printf("DEBUGGING AXI4 STREAMING DATA TYPES!\r\n");

    // prepare data for the DUT
    hls::stream<AXI_VAL> istream;
    hls::stream<AXI_VAL> ostream;

    // stream in the offset vector
PACK_OFF : {
    assert(CLASSES < OUT_WIDTH_RATIO);
    axi_T packet;
    for (int i = 0; i < CLASSES; i++) {
        packet.o[i] = offsets[i];
    }
    push_stream(istream, packet, 0);
}

    // stream in the weigth matrix
PACK_W1:
    assert(W_WIDTH_RATIO <= FEAT);
    for (int i = 0; i < HIDDEN; i++) {
        for (int j = 0; j < FEAT; j += W_WIDTH_RATIO) {
            axi_T packet;
            for (int w = 0; w < W_WIDTH_RATIO; w++) {
                packet.w[w] = weight1[i][j + w];
            };
            push_stream(istream, packet, 0);
        }
    }

PACK_W2:
    assert(W_WIDTH_RATIO <= HIDDEN);
    for (int i = 0; i < CLASSES; i++) {
        for (int j = 0; j < HIDDEN; j += W_WIDTH_RATIO) {
            axi_T packet;
            for (int w = 0; w < W_WIDTH_RATIO; w++) {
                packet.w[w] = weight2[i][j + w];
            };
            push_stream(istream, packet, 0);
        }
    }

    // stream in the input matrix
PACK_IN:
    assert(IN_WIDTH_RATIO <= FEAT);
    for (int i = 0; i < BATCH; i++) {
        for (int j = 0; j < FEAT; j += IN_WIDTH_RATIO) {
            axi_T packet;
            for (int w = 0; w < IN_WIDTH_RATIO / 2; w++) {
                packet.i[w].a0 = (uint8_t)inputs[i][j + 2 * w];
                packet.i[w].a1 = (uint8_t)inputs[i][j + 2 * w + 1];
            };
            push_stream(istream, packet, (i == BATCH - 1) && (j == FEAT - IN_WIDTH_RATIO));
        }
    }

    // call the DUT
    mmult_hw(istream, ostream);

    // extract the output matrix from the out stream
    assert(CLASSES <= OUT_WIDTH_RATIO);
UNPACK_OUT : {
    for (int i = 0; i < BATCH; i++) {
        axi_T packet = pop_stream(ostream);
        for (int j = 0; j < CLASSES; j++) {
            output_hw[i][j] = packet.o[j];
        }
    }
}
    /* reference Matrix Multiplication */
    matrix_multiply_ref(inputs, weight1, weight2, offsets, output_sw);

    /** Matrix comparison */
    err = 0;
    for (i = 0; i < BATCH; i++) {
        for (j = 0; j < CLASSES; j++) {
            if (output_sw[i][j] != output_hw[i][j]) {
                err++;
                std::cout << i << "," << j << ": expected " << output_sw[i][j] << " but got "
                          << output_hw[i][j] << std::endl;
            }
        }
    }

    if (err == 0)
        printf("Matrices identical ... Test successful!\r\n");
    else
        printf("Test failed!\r\n");

    return err;
}
