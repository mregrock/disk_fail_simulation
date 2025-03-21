#include <gtest/gtest.h>
#include "model/vdisk.h"
#include "model/pdisk.h" 
#include "model/group.h" 



TEST(TVDiskTest, InitialStateAndAssign) {
    arctic::TVDiskId vdiskId = arctic::TVDiskId::FromValue(20);
    arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(5);
    arctic::TDCId dcId(0);
    arctic::TVDisk vdisk(vdiskId, pdiskId, dcId);

    ASSERT_EQ(vdisk.GetId(), vdiskId);
    ASSERT_EQ(vdisk.GetPDiskId(), pdiskId);
    ASSERT_EQ(vdisk.GetDCId(), dcId);
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Active);
    ASSERT_EQ(vdisk.GetGroupId(), arctic::TGroupId::Zero());
    ASSERT_FALSE(vdisk.IsReplicationTriggered());
    ASSERT_EQ(vdisk.GetReplicationCompleteTime(), 0.0);

    arctic::TGroupId groupId = arctic::TGroupId::FromValue(101);
    vdisk.AssignToGroup(groupId);
    ASSERT_EQ(vdisk.GetGroupId(), groupId);
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Active);
}

TEST(TVDiskTest, MarkReplication) {
    arctic::TVDiskId vdiskId = arctic::TVDiskId::FromValue(21);
    arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(6);
    arctic::TDCId dcId(1);
    arctic::TVDisk vdisk(vdiskId, pdiskId, dcId);

    vdisk.MarkReplicationTriggered(100.0);
    ASSERT_FALSE(vdisk.IsReplicationTriggered());
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Active);
    ASSERT_EQ(vdisk.GetReplicationCompleteTime(), 0.0);

    vdisk.SetState(arctic::TVDisk::Faulty);
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Faulty);

    double completeTime = 200.0;
    vdisk.MarkReplicationTriggered(completeTime);
    ASSERT_TRUE(vdisk.IsReplicationTriggered());
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Replicating);
    ASSERT_EQ(vdisk.GetReplicationCompleteTime(), completeTime);

    vdisk.MarkReplicationTriggered(completeTime + 50.0);
    ASSERT_TRUE(vdisk.IsReplicationTriggered());
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Replicating);
    ASSERT_EQ(vdisk.GetReplicationCompleteTime(), completeTime);

    vdisk.SetState(arctic::TVDisk::Faulty);
    vdisk.MarkReplicationTriggered(0.0);
    ASSERT_TRUE(vdisk.IsReplicationTriggered());
    ASSERT_EQ(vdisk.GetState(), arctic::TVDisk::Faulty);
    ASSERT_EQ(vdisk.GetReplicationCompleteTime(), 0.0);
} 