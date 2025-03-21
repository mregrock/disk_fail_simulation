#include <gtest/gtest.h>
#include "model/pdisk.h"
#include "model/vdisk.h"
#include "model/simulation_params.h" 
#include <memory> 

namespace arctic {
    extern Ui32 GVDisksPerPDisk;
    extern Ui32 GSpareDisksPerDc; 
}

TEST(TPDiskTest, InitialState) {
    arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(1);
    arctic::TDCId dcId(0);
    arctic::TPDisk pdisk(pdiskId, dcId, arctic::TPDisk::Active);

    ASSERT_EQ(pdisk.GetState(), arctic::TPDisk::Active);
    ASSERT_EQ(pdisk.GetId(), pdiskId);
    ASSERT_EQ(pdisk.GetDCId(), dcId);
    ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), arctic::GVDisksPerPDisk);
    ASSERT_EQ(pdisk.GetBrokenTime(), 0.0);
}

TEST(TPDiskTest, FailAndRecover) {
    arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(2);
    arctic::TDCId dcId(1);
    auto pdisk = std::make_shared<arctic::TPDisk>(pdiskId, dcId, arctic::TPDisk::Active);

    auto vdisk1 = std::make_shared<arctic::TVDisk>(arctic::TVDiskId::FromValue(10), pdiskId, dcId);
    auto vdisk2 = std::make_shared<arctic::TVDisk>(arctic::TVDiskId::FromValue(11), pdiskId, dcId);
    pdisk->AddVDisk(vdisk1);
    pdisk->AddVDisk(vdisk2);
    vdisk2->SetState(arctic::TVDisk::Faulty);

    double failTime = 123.5;
    pdisk->Fail(failTime);

    ASSERT_EQ(pdisk->GetState(), arctic::TPDisk::Broken);
    ASSERT_EQ(pdisk->GetAvailableVDiskSlots(), 0);
    ASSERT_EQ(pdisk->GetBrokenTime(), failTime);
    ASSERT_EQ(vdisk1->GetState(), arctic::TVDisk::Faulty);
    ASSERT_EQ(vdisk2->GetState(), arctic::TVDisk::Faulty);

    pdisk->Fail(failTime + 10.0);
    ASSERT_EQ(pdisk->GetState(), arctic::TPDisk::Broken);
    ASSERT_EQ(pdisk->GetBrokenTime(), failTime);

    pdisk->Recover();
    ASSERT_EQ(pdisk->GetState(), arctic::TPDisk::Spare);
    ASSERT_EQ(pdisk->GetAvailableVDiskSlots(), arctic::GVDisksPerPDisk);
    ASSERT_EQ(pdisk->GetBrokenTime(), 0.0);
    ASSERT_EQ(vdisk1->GetState(), arctic::TVDisk::Faulty);
    ASSERT_EQ(vdisk2->GetState(), arctic::TVDisk::Faulty);
}

TEST(TPDiskTest, DecrementSlots) {
     arctic::TPDiskId pdiskId = arctic::TPDiskId::FromValue(3);
     arctic::TDCId dcId(2);
     arctic::TPDisk pdisk(pdiskId, dcId, arctic::TPDisk::Spare);

     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), arctic::GVDisksPerPDisk);
     pdisk.DecrementAvailableVDiskSlots();
     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), arctic::GVDisksPerPDisk - 1);

     for (int i = 0; i < arctic::GVDisksPerPDisk - 1; ++i) {
         pdisk.DecrementAvailableVDiskSlots();
     }
     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), 0);

     pdisk.DecrementAvailableVDiskSlots();
     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), 0);

     pdisk.Fail(10.0);
     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), 0);
     pdisk.DecrementAvailableVDiskSlots();
     ASSERT_EQ(pdisk.GetAvailableVDiskSlots(), 0);
}
