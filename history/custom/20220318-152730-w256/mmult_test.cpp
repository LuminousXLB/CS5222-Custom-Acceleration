#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "mmult.h"

void matrix_multiply_ref(
    out_T offsets[CLASSES],
    w_T weights[CLASSES][FEAT],
    in_T in[BATCH][FEAT],
    out_T out[BATCH][CLASSES])
{
    // matrix multiplication of a A*B matrix
    for (int i = 0; i < BATCH; ++i) {
        for (int j = 0; j < CLASSES; ++j) {
            out_T sum = offsets[j];
            for (int k = 0; k < FEAT; ++k) {
                sum += in[i][k] * weights[j][k];
            }
            out[i][j] = sum;
        }
    }
    return;
}

int main(void)
{
    int i, j, err;

    out_T offsets[CLASSES];
    w_T weights[CLASSES][FEAT];
    in_T inputs[BATCH][FEAT];
    out_T output_sw[BATCH][CLASSES];
    out_T output_hw[BATCH][CLASSES];

    /** Matrix Initiation */
    for (i = 0; i < CLASSES; i++) {
        offsets[i] = (out_T)((rand() % (1ULL << OUT_WIDTH)) - (1ULL << (OUT_WIDTH - 1)));
    }

    for (i = 0; i < CLASSES; i++) {
        for (j = 0; j < FEAT; j++) {
            weights[i][j] = (w_T)((rand() % (1ULL << W_WIDTH)) - (1ULL << (W_WIDTH - 1)));
        }
    }

    for (i = 0; i < BATCH; i++) {
        for (j = 0; j < FEAT; j++) {
            inputs[i][j] = (in_T)(rand() % (1ULL << IN_WIDTH));
        }
    }
    /** End of Initiation */

    printf("DEBUGGING AXI4 STREAMING DATA TYPES!\r\n");

    // prepare data for the DUT
    hls::stream<AXI_VAL> in_stream;
    hls::stream<AXI_VAL> out_stream;

    // stream in the offset vector
    for (int i = 0; i < CLASSES; i += OUT_WIDTH_RATIO) {
        axi_T packet;
    PACK_OFF:
        for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
            packet.o[w] = offsets[i + w];
        };
        in_stream.write(push_stream(packet, 0));
    }

    // stream in the weigth matrix
    for (int i = 0; i < CLASSES; i++) {
        for (int j = 0; j < FEAT; j += W_WIDTH_RATIO) {
            axi_T packet;
        PACK_W:
            for (int w = 0; w < W_WIDTH_RATIO; w++) {
                packet.w[w] = weights[i][j + w];
            };
            in_stream.write(push_stream(packet, 0));
        }
    }

    // stream in the input matrix
    for (int i = 0; i < BATCH; i++) {
        for (int j = 0; j < FEAT; j += IN_WIDTH_RATIO) {
            axi_T packet;
        PACK_IN:
            for (int w = 0; w < IN_WIDTH_RATIO; w++) {
                packet.i[w] = inputs[i][j + w];
            };
            in_stream.write(push_stream(packet, (i == BATCH - 1) && (j == FEAT - 1)));
        }
    }

    // call the DUT
    mmult_hw(in_stream, out_stream);

    AXI_VAL tmp;

    // extract the output matrix from the out stream
    for (int i = 0; i < BATCH; i++) {
        for (int j = 0; j < CLASSES; j += OUT_WIDTH_RATIO) {
            out_stream.read(tmp);
            axi_T packet = pop_stream(tmp);
        UNPACK_OUT:
            for (int w = 0; w < OUT_WIDTH_RATIO; w++) {
                output_hw[i][j + w] = packet.o[w];
            }
        }
    }

    /* reference Matrix Multiplication */
    matrix_multiply_ref(offsets, weights, inputs, output_sw);

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
