#include <ap_axi_sdata.h>
#include <assert.h>
#include <hls_stream.h>
#include <stdint.h>

// Datatype widths in bits
#define W_WIDTH 8
#define IN_WIDTH 4
#define OUT_WIDTH 16

// Type definition of matrix elements
typedef ap_int<W_WIDTH> w_T;
typedef ap_uint<IN_WIDTH> in_T;
typedef ap_int<OUT_WIDTH> out_T;

// Data type ratio between data type and axi width
#define AXI_WIDTH (256)

#define W_WIDTH_RATIO (AXI_WIDTH / W_WIDTH)
#define IN_WIDTH_RATIO (AXI_WIDTH / IN_WIDTH)
#define OUT_WIDTH_RATIO (AXI_WIDTH / OUT_WIDTH)

// AXI width
union axi_T {
    int8_t w[W_WIDTH_RATIO];
    struct {
        uint8_t a0 : 4;
        uint8_t a1 : 4;
    } i[IN_WIDTH_RATIO / 2];
    int16_t o[OUT_WIDTH_RATIO];
};

// Matrix dimensions specifications
#define BATCH 8192
#define FEAT 64
#define HIDDEN 32
#define CLASSES 10
#define TILING 128

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
