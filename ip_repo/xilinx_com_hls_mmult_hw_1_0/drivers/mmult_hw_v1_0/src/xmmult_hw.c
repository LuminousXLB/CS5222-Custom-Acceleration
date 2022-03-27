// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2020.2 (64-bit)
// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xmmult_hw.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XMmult_hw_CfgInitialize(XMmult_hw *InstancePtr, XMmult_hw_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_bus_BaseAddress = ConfigPtr->Control_bus_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XMmult_hw_Start(XMmult_hw *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL) & 0x80;
    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL, Data | 0x01);
}

u32 XMmult_hw_IsDone(XMmult_hw *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XMmult_hw_IsIdle(XMmult_hw *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XMmult_hw_IsReady(XMmult_hw *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XMmult_hw_EnableAutoRestart(XMmult_hw *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL, 0x80);
}

void XMmult_hw_DisableAutoRestart(XMmult_hw *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_AP_CTRL, 0);
}

void XMmult_hw_InterruptGlobalEnable(XMmult_hw *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_GIE, 1);
}

void XMmult_hw_InterruptGlobalDisable(XMmult_hw *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_GIE, 0);
}

void XMmult_hw_InterruptEnable(XMmult_hw *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_IER);
    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_IER, Register | Mask);
}

void XMmult_hw_InterruptDisable(XMmult_hw *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_IER);
    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_IER, Register & (~Mask));
}

void XMmult_hw_InterruptClear(XMmult_hw *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMmult_hw_WriteReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_ISR, Mask);
}

u32 XMmult_hw_InterruptGetEnabled(XMmult_hw *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_IER);
}

u32 XMmult_hw_InterruptGetStatus(XMmult_hw *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMmult_hw_ReadReg(InstancePtr->Control_bus_BaseAddress, XMMULT_HW_CONTROL_BUS_ADDR_ISR);
}

