#include <ap_axi_sdata.h>
#include <assert.h>
#include <hls_stream.h>
#include <stdint.h>

// Datatype widths in bits
#define IN_WIDTH 4
#define W1_WIDTH 8
#define W2_WIDTH 8
#define OUT_WIDTH 8

// Type definition of matrix elements
typedef ap_uint<IN_WIDTH> in_T;
typedef ap_int<W1_WIDTH> w1_T;
typedef ap_int<W2_WIDTH> w2_T;
typedef ap_int<OUT_WIDTH> out_T;

// Data type ratio between data type and axi width
#define AXI_WIDTH (256)

#define IN_WIDTH_RATIO (AXI_WIDTH / IN_WIDTH)
#define W1_WIDTH_RATIO (AXI_WIDTH / W1_WIDTH)
#define W2_WIDTH_RATIO (AXI_WIDTH / W2_WIDTH)
#define OUT_WIDTH_RATIO (AXI_WIDTH / OUT_WIDTH)

// AXI width
union axi_T {
    struct {
        uint8_t a0 : 4;
        uint8_t a1 : 4;
    } i[IN_WIDTH_RATIO / 2];
#if W1_WIDTH == 4
    struct {
        int8_t a0 : 4;
        int8_t a1 : 4;
    } w1[W1_WIDTH_RATIO];
#else
    int8_t w1[W1_WIDTH_RATIO];
#endif
    int8_t w2[W2_WIDTH_RATIO];
    int16_t o[OUT_WIDTH_RATIO];
};

// Matrix dimensions specifications
#define BATCH 8192
#define FEAT 64
#define HIDDEN 16
#define CLASSES 10
#define TILING 32

// AXI settings (leave it fixed)
#define AXI_DATA (sizeof(axi_T) * 8)
#define AXI_U 4
#define AXI_TI 5
#define AXI_TD 5

// AXI interface
typedef ap_axiu<AXI_DATA, AXI_U, AXI_TI, AXI_TD> AXI_VAL;

// Matrix Multiply prototype
void mmult_hw(hls::stream<AXI_VAL> &in_stream, hls::stream<AXI_VAL> &out_stream);

// AXI stream push and pop
axi_T pop_stream(hls::stream<AXI_VAL> &is);
void push_stream(hls::stream<AXI_VAL> &is, axi_T const &v, bool last);
