//
//  ops-test.cpp
//  Katara
//
//  Created by Arne Philipeit on 5/2/20.
//  Copyright © 2020 Arne Philipeit. All rights reserved.
//

#include "gtest/gtest.h"

#include "ops.h"

namespace x86_64 {

TEST(RegTest, RegEncodeInOpcode) {
    struct Test {
        Size size_;
        uint8_t reg_;
        
        bool exp_requires_rex_;
        uint8_t exp_rex_;
        uint8_t exp_opcode_;
    };
    Test tests[] = {
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x01
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x07
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 12,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_opcode_= */ 0x04
        },
        Test{
            /* size_= */ Size::k16,
            /* reg_= */ 4,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x04
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 3,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x03
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 8,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_opcode_= */ 0x00
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x01
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_opcode_= */ 0x07
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 13,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_opcode_= */ 0x05
        },
    };
    
    for (Test test : tests) {
        Reg reg(test.size_, test.reg_);
        
        EXPECT_EQ(reg.RequiresREX(), test.exp_requires_rex_);
        
        uint8_t rex = 0x00;
        uint8_t opcode = 0x00;
        
        reg.EncodeInOpcode(&rex, &opcode, 0);
        
        EXPECT_EQ(rex, test.exp_rex_);
        EXPECT_EQ(opcode, test.exp_opcode_);
    }
}

TEST(RegTest, RegEncodeInModRM_SIB_Disp) {
    struct Test {
        Size size_;
        uint8_t reg_;
        
        bool exp_requires_rex_;
        uint8_t exp_rex_;
        uint8_t exp_modrm_;
    };
    Test tests[] = {
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc1
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc7
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 12,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_modrm_= */ 0xc4
        },
        Test{
            /* size_= */ Size::k16,
            /* reg_= */ 4,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc4
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 3,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc3
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 8,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_modrm_= */ 0xc0
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc1
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0xc7
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 13,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x01,
            /* exp_modrm_= */ 0xc5
        },
    };
    
    for (Test test : tests) {
        Reg reg(test.size_, test.reg_);
        
        EXPECT_EQ(reg.RequiresREX(), test.exp_requires_rex_);
        
        uint8_t rex = 0x00;
        uint8_t modrm = 0x00;
        uint8_t sib = 0x00;
        uint8_t disp = 0x00;
        
        reg.EncodeInModRM_SIB_Disp(&rex, &modrm, &sib, &disp);
        
        EXPECT_EQ(rex, test.exp_rex_);
        EXPECT_EQ(modrm, test.exp_modrm_);
        EXPECT_EQ(sib, 0x00);
        EXPECT_EQ(disp, 0x00);
    }
}

TEST(RegTest, RegEncodeInModRMReg) {
    struct Test {
        Size size_;
        uint8_t reg_;
        
        bool exp_requires_rex_;
        uint8_t exp_rex_;
        uint8_t exp_modrm_;
    };
    Test tests[] = {
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x08
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x38
        },
        Test{
            /* size_= */ Size::k8,
            /* reg_= */ 12,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x04,
            /* exp_modrm_= */ 0x20
        },
        Test{
            /* size_= */ Size::k16,
            /* reg_= */ 4,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x20
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 3,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x18
        },
        Test{
            /* size_= */ Size::k32,
            /* reg_= */ 8,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x04,
            /* exp_modrm_= */ 0x00
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 1,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x08
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 7,
            /* exp_requires_rex_= */ false,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x38
        },
        Test{
            /* size_= */ Size::k64,
            /* reg_= */ 13,
            /* exp_requires_rex_= */ true,
            /* exp_rex_= */ 0x04,
            /* exp_modrm_= */ 0x28
        },
    };
    
    for (Test test : tests) {
        Reg reg(test.size_, test.reg_);
        
        EXPECT_EQ(reg.RequiresREX(), test.exp_requires_rex_);
        
        uint8_t rex = 0x00;
        uint8_t modrm = 0x00;
        
        reg.EncodeInModRMReg(&rex, &modrm);
        
        EXPECT_EQ(rex, test.exp_rex_);
        EXPECT_EQ(modrm, test.exp_modrm_);
    }
}


TEST(MemTest, MemEncodeInModRM_SIB_Disp) {
    struct Test {
        Size size_;
        uint8_t base_reg_;
        uint8_t index_reg_;
        Scale scale_;
        int32_t disp_;
        
        bool exp_requires_rex_;
        bool exp_requires_sib_;
        uint8_t exp_required_disp_size_;
        uint8_t exp_rex_;
        uint8_t exp_modrm_;
        uint8_t exp_sib_;
        uint8_t exp_disp_[4];
    };
    Test tests[] = {
        Test{
            /* size_= */ Size::k8,
            /* base_reg_= */ 0,
            /* index_reg_= */ 0,
            /* scale_= */ Scale::kS00,
            /* disp_= */ 0,
            /* exp_requires_rex_= */ false,
            /* exp_requires_sib_= */ true,
            /* exp_disp_size_= */ 0,
            /* exp_rex_= */ 0x00,
            /* exp_modrm_= */ 0x04,
            /* exp_sib_= */ 0x00,
            /* exp_disp_= */ {0x00, 0x00, 0x00, 0x00}
        },
    };
    
    for (Test test : tests) {
        Mem mem(test.size_,
                test.base_reg_,
                test.index_reg_,
                test.scale_,
                test.disp_);
        
        EXPECT_EQ(mem.RequiresREX(), test.exp_requires_rex_);
        EXPECT_EQ(mem.RequiresSIB(), test.exp_requires_sib_);
        EXPECT_EQ(mem.RequiredDispSize(), test.exp_required_disp_size_);
        
        uint8_t rex = 0x00;
        uint8_t modrm = 0x00;
        uint8_t sib = 0x00;
        uint8_t disp[4]{0x00, 0x00, 0x00, 0x00};
        
        mem.EncodeInModRM_SIB_Disp(&rex, &modrm, &sib, disp);
        
        EXPECT_EQ(rex, test.exp_rex_);
        EXPECT_EQ(modrm, test.exp_modrm_);
        EXPECT_EQ(sib, test.exp_sib_);
        for (int i = 0; i < 4; i++) {
            EXPECT_EQ(disp[i], test.exp_disp_[i]);
        }
    }
}

}
