#include <gtest/gtest.h>
#include "model/group.h"
#include "model/vdisk.h"
#include "model/pdisk.h" 
#include <memory> 
#include <unordered_map> 

class TGroupTest : public ::testing::Test {
protected:
    std::unordered_map<arctic::TVDiskId, std::shared_ptr<arctic::TVDisk>> vdiskMap;
    std::shared_ptr<arctic::TGroup> group;
    arctic::TGroupId groupId;

    void SetUp() override {
        groupId = arctic::TGroupId::FromValue(1);
        group = std::make_shared<arctic::TGroup>(groupId);
        for (int dc = 0; dc < 3; ++dc) {
            for (int i = 0; i < 3; ++i) {
                arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(dc * 10 + i);
                arctic::TVDiskId vdiskId = arctic::TVDiskId::FromValue(dc * 3 + i);
                auto vdisk = std::make_shared<arctic::TVDisk>(vdiskId, pdiskId, arctic::TDCId(dc));
                vdisk->AssignToGroup(groupId);
                vdiskMap[vdiskId] = vdisk;
                group->AddVDisk(vdisk, arctic::TDCId(dc));
            }
        }
    }

    void SetVDiskState(arctic::Ui32 vdiskRawId, arctic::TVDisk::VDiskState state) {
        arctic::TVDiskId vdiskId = arctic::TVDiskId::FromValue(vdiskRawId);
        auto it = vdiskMap.find(vdiskId);
        if (it != vdiskMap.end()) {
            it->second->SetState(state);
        }
    }
};

TEST_F(TGroupTest, CheckDataLoss_NoFailures) {
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_SingleFailureDC0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_SingleFailureDC1) {
    SetVDiskState(3, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_SingleFailureDC2) {
    SetVDiskState(6, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_OneFailureEachDC) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(4, arctic::TVDisk::Faulty);
    SetVDiskState(8, arctic::TVDisk::Faulty);
    ASSERT_TRUE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_TwoFailuresDC0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_TwoFailuresDC1) {
    SetVDiskState(3, arctic::TVDisk::Faulty);
    SetVDiskState(5, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_TwoFailuresDC2) {
    SetVDiskState(6, arctic::TVDisk::Faulty);
    SetVDiskState(7, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_ThreeFailuresDC0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(2, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_OneFailureDC0_OneFailureDC1) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_OneFailureDC0_OneFailureDC2) {
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(8, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_OneFailureDC1_OneFailureDC2) {
    SetVDiskState(5, arctic::TVDisk::Faulty);
    SetVDiskState(7, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_ReplicatingCountsAsFailure1) {
    SetVDiskState(0, arctic::TVDisk::Replicating);
    SetVDiskState(3, arctic::TVDisk::Replicating);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_ReplicatingCountsAsFailure2) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Replicating);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_ReplicatedDoesNotCountAsFailure) {
    SetVDiskState(0, arctic::TVDisk::Replicated);
    SetVDiskState(3, arctic::TVDisk::Replicated);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_MaxTolerable_3_1_0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(2, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_MaxTolerable_2_2_0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    SetVDiskState(4, arctic::TVDisk::Faulty);
    ASSERT_FALSE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_LossCase_3_2_0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(2, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    SetVDiskState(4, arctic::TVDisk::Faulty);
    ASSERT_TRUE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_LossCase_3_3_0) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(2, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    SetVDiskState(4, arctic::TVDisk::Faulty);
    SetVDiskState(5, arctic::TVDisk::Faulty);
    ASSERT_TRUE(group->CheckDataLoss());
}

TEST_F(TGroupTest, CheckDataLoss_LossCase_2_1_1) {
    SetVDiskState(0, arctic::TVDisk::Faulty);
    SetVDiskState(1, arctic::TVDisk::Faulty);
    SetVDiskState(3, arctic::TVDisk::Faulty);
    SetVDiskState(6, arctic::TVDisk::Faulty);
    ASSERT_TRUE(group->CheckDataLoss());
}
