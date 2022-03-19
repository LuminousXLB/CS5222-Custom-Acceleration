#include <ap_axi_sdata.h>
#include <assert.h>
#include <hls_stream.h>
#include <stdint.h>

// Type definition of matrix elements
typedef ap_int<8> w_T;
typedef ap_uint<8> in_T;
typedef ap_int<32> out_T;
// // Equivalent-width integer types
// typedef unsigned char w_bit_T;
// typedef unsigned char in_bit_T;
// typedef unsigned out_bit_T;

// Datatype widths in bits
#define W_WIDTH (sizeof(w_T) * 8)
#define IN_WIDTH (sizeof(in_T) * 8)
#define OUT_WIDTH (sizeof(out_T) * 8)

// Data type ratio between data type and axi width
#define AXI_WIDTH (128)

#define W_WIDTH_RATIO (AXI_WIDTH / W_WIDTH)
#define IN_WIDTH_RATIO (AXI_WIDTH / IN_WIDTH)
#define OUT_WIDTH_RATIO (AXI_WIDTH / OUT_WIDTH)

// AXI width
// typedef unsigned long long axi_T;
union axi_T {
    int8_t w[W_WIDTH_RATIO];
    uint8_t i[IN_WIDTH_RATIO];
    int32_t o[OUT_WIDTH_RATIO];
};

// Matrix dimensions specifications
#define BATCH 8192
#define FEAT 256
#define CLASSES 10

// Input matrix tiling factor
// CSE548: TODO
#define TILING 128

// Input/Output Stream Size
#define IS_SIZE                                                                           \
    ((CLASSES + OUT_WIDTH_RATIO - 1) / OUT_WIDTH_RATIO + CLASSES * FEAT / W_WIDTH_RATIO + \
     BATCH * FEAT / IN_WIDTH_RATIO)
#define OS_SIZE (BATCH * ((CLASSES + OUT_WIDTH_RATIO - 1) / OUT_WIDTH_RATIO))

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
axi_T pop_stream(AXI_VAL const &e);
AXI_VAL push_stream(axi_T const &v, bool last);
