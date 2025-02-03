#include "engine/easy.h"
#include <map>
#include <iomanip>

using namespace arctic;

std::shared_ptr<GuiTheme> GTheme;
std::shared_ptr<Panel> GGui;
Font GFont;

std::shared_ptr<Text> GTextDisksPerDc;
std::shared_ptr<Text> GTextDiskSize;
std::shared_ptr<Text> GTextFailureRate;
std::shared_ptr<Text> GTextDataLoss;
std::shared_ptr<Text> GTextStats;
std::shared_ptr<Text> GTextSpareDisks;
std::shared_ptr<Text> GTextWriteSpeed;

std::shared_ptr<Scrollbar> GScrollDisksPerDc;
std::shared_ptr<Scrollbar> GScrollDiskSize;
std::shared_ptr<Scrollbar> GScrollFailureRate;
std::shared_ptr<Scrollbar> GScrollSpareDisks;
std::shared_ptr<Scrollbar> GScrollWriteSpeed;

Ui32 GDisksPerDc = 100;
Ui32 GDiskSize = 4096;
Ui32 GFailureRate = 3;
Ui32 GSpareDisksPerDc = 10;
Ui32 GWriteSpeed = 100;
Ui64 GSims = 0;

double GDataLossProb = 0.0;
bool GDoRestart = true;

const Ui32 kGraphWidth = 1200;
const Ui32 kGraphHeight = 500;
const Ui32 kScreenWidth = 1920;
const Ui32 kScreenHeight = 1080;
const Ui32 kLeftMargin = (kScreenWidth - kGraphWidth) / 2;

const Ui32 kTopMargin = (kScreenHeight - kGraphHeight) / 2;
const Ui32 kControlsYOffset = 0;

struct Disk {
    enum DiskState { Active, Inactive, Faulty };
    DiskState State = Active;
    double InactiveTime = 0;
    double RebuildTime = 0;
    Ui32 DcId = 0;
    Si32 GroupId = -1;
};

struct DataCenter {
    std::vector<Disk> Disks;
    std::vector<Ui32> SpareDisks;
    std::unordered_set<Ui32> InactiveDisks;
    std::unordered_set<Ui32> FaultyDisks;
};

struct Sim {
    std::vector<DataCenter> Dcs;
    double CurrentTime = 0;
    Ui32 LostData = 0;

    void Reset() {
        Dcs.resize(3);
        CurrentTime = 0;
        LostData = 0;
        
        for (Si32 dc = 0; dc < 3; ++dc) {
            Dcs[dc].Disks.resize(GDisksPerDc + GSpareDisksPerDc);
            Dcs[dc].SpareDisks.clear();
            
            for (Si32 i = 0; i < GDisksPerDc; ++i) {
                Dcs[dc].Disks[i].State = Disk::Active;
                Dcs[dc].Disks[i].DcId = dc;
            }
            
            for (Si32 i = GDisksPerDc; i < GDisksPerDc + GSpareDisksPerDc; ++i) {
                Dcs[dc].SpareDisks.push_back(i);
                Dcs[dc].Disks[i].State = Disk::Inactive;
                Dcs[dc].Disks[i].DcId = dc;
            }
        }
        
        Si32 groupId = 0;
        std::vector<bool> usedDisks[3];
        for (Si32 dc = 0; dc < 3; ++dc) {
            usedDisks[dc].resize(GDisksPerDc, false);
        }
        
        while (true) {
            bool found = false;
            Si32 diskIndices[3];
            
            for (Si32 dc = 0; dc < 3; ++dc) {
                for (Si32 i = 0; i < GDisksPerDc; ++i) {
                    if (!usedDisks[dc][i]) {
                        diskIndices[dc] = i;
                        found = true;
                        break;
                    }
                }
                if (!found) break;
            }
            
            if (!found) break;
            
            for (Si32 dc = 0; dc < 3; ++dc) {
                usedDisks[dc][diskIndices[dc]] = true;
                Dcs[dc].Disks[diskIndices[dc]].GroupId = groupId;
            }
            groupId++;
        }
    }

    void SimulateHour() {
        double failuresThisHour = GFailureRate / 24.0;
        Si32 failures = static_cast<Si32>(failuresThisHour);
        

        if (Random32() % 1000000 < (failuresThisHour - failures) * 1000000) {
            failures++;
        }

        
        for (Si32 i = 0; i < failures; ++i) {
            Si32 attempts = 0;

            while (attempts < 100) {
                Si32 dcIndex = Random32() % 3;
                Si32 diskIndex = Random32() % GDisksPerDc;
                

                Disk& disk = Dcs[dcIndex].Disks[diskIndex];
                if (disk.State == Disk::Active && disk.GroupId >= 0) {
                    disk.State = Disk::Inactive;
                    disk.InactiveTime = CurrentTime;
                    break;
                }
                attempts++;
            }
        }

        std::map<Si32, std::vector<Disk*>> disksByGroup;

        for (auto& dc : Dcs) {
            for (auto& disk : dc.Disks) {
                if (disk.GroupId >= 0) {
                    disksByGroup[disk.GroupId].push_back(&disk);
                }
            }
        }

        for (const auto& group : disksByGroup) {
            const auto& groupDisks = group.second;
            

            Si32 activeDisks = 0;
            Si32 inactiveDisks = 0;
            Si32 faultyDisks = 0;
            
            for (const auto* disk : groupDisks) {
                if (disk->State == Disk::Active) activeDisks++;
                else if (disk->State == Disk::Inactive) inactiveDisks++;
                else if (disk->State == Disk::Faulty) faultyDisks++;
            }
            if (inactiveDisks > 0) {
                for (auto* disk : groupDisks) {
                    if (disk->State == Disk::Inactive) {

                        if (CurrentTime >= disk->InactiveTime + 1.0) {
                            auto& dc = Dcs[disk->DcId];
                            if (!dc.SpareDisks.empty()) {

                                Si32 spareIndex = dc.SpareDisks.back();
                                dc.SpareDisks.pop_back();
                                
                                Disk& newDisk = dc.Disks[spareIndex];
                                newDisk.State = Disk::Inactive;
                                newDisk.GroupId = group.first;
                                
                                double sizeMb = GDiskSize * 1024.0;
                                newDisk.RebuildTime = CurrentTime + (sizeMb / GWriteSpeed) / 3600.0;
                                

                                disk->State = Disk::Faulty;
                            } else {
                                disk->State = Disk::Faulty;
                                faultyDisks++;
                            }

                        }
                    }
                }
            }
            
            if (activeDisks == 0 && (faultyDisks + inactiveDisks) == groupDisks.size()) {
                LostData++;
            }

        }

        for (auto& dc : Dcs) {
            for (auto& disk : dc.Disks) {
                if (disk.State == Disk::Inactive && CurrentTime >= disk.RebuildTime) {

                    disk.State = Disk::Active;
                }
            }
        }
        
        CurrentTime += 1.0;
    }
};

Sim GSim;

std::map<Si32, Si32> DataLossByDay;
std::map<Si32, Si32> TotalSimsByDay;

void DrawCumulative(std::map<Si64, Si64>& valuesByCount, Rgba color, 
                   Vec2Si32 position, Vec2Si32 size, const char* title) {
    std::vector<Vec2D> points;
    points.reserve(valuesByCount.size() + 2);

    points.emplace_back(0.0, 0.0);

    double totalSum = 0.0;
    for (const auto& pair : valuesByCount) {
        totalSum += pair.second;
        points.emplace_back(pair.first, totalSum);
    }
    
    if (points.size() > 1) {
        if (totalSum > 0.0) {
            for (auto& point : points) {
                point.y /= totalSum;
            }
        }

        DrawRectangle(position, position + size, Rgba(32,32,32));

        Vec2D scale = Vec2D(double(size.x) / 30.0, size.y);
        Vec2D point0 = points[0];
        
        for (Si32 i = 1; i < points.size(); ++i) {
            Vec2D point1 = points[i];
            DrawLine(position + Vec2Si32(scale.x * point0.x, scale.y * point0.y),
                    position + Vec2Si32(scale.x * point1.x, scale.y * point1.y),
                    color);
            point0 = point1;
        }

        GFont.Draw(title, position.x, position.y + size.y + 50, kTextOriginBottom);
        GFont.Draw("Days", position.x + size.x/2, position.y - 20, kTextOriginTop);
        GFont.Draw("Probability", position.x - 20, position.y + size.y/2, kTextOriginTop);

        GFont.Draw("0", position.x, position.y, kTextOriginTop);
        GFont.Draw("30", position.x + size.x, position.y, kTextOriginTop);

        Si32 mouseX = MouseX();
        Si32 viewMouseX = mouseX - position.x;
        if (viewMouseX >= 0 && viewMouseX < size.x) {
            Si32 currentDay = viewMouseX * 30 / size.x;
            
            Si32 nearestIdx = 0;
            for (Si32 i = 0; i < points.size(); ++i) {
                if (points[i].x > currentDay) {
                    nearestIdx = std::max(0, i-1);
                    break;
                }
            }
            
            DrawLine(position + Vec2Si32(viewMouseX, 0),
                    position + Vec2Si32(viewMouseX, size.y),
                    Rgba(128, 128, 128));

            std::stringstream str;
            str << std::fixed << std::setprecision(6)
                << "Day " << currentDay 
                << "\nProbability of data loss: " 
                << (points[nearestIdx].y * 100.0) << "%";
            GFont.Draw(str.str().c_str(),
                      mouseX,
                      position.y + Si32(scale.y * points[nearestIdx].y),
                      kTextOriginTop);
        }
    }
}

void UpdateText() {
    using namespace std;
    
    if (GDisksPerDc != GScrollDisksPerDc->GetValue()) {
        GDisksPerDc = GScrollDisksPerDc->GetValue();
        GDoRestart = true;

        stringstream str;
        str << "Disks per DC: " << GDisksPerDc;
        GTextDisksPerDc->SetText(str.str());
    }
    
    if (GDiskSize != GScrollDiskSize->GetValue()) {
        GDiskSize = GScrollDiskSize->GetValue();
        GDoRestart = true;
        stringstream str;
        str << "Disk Size: " << GDiskSize << " GB";
        GTextDiskSize->SetText(str.str());
    }
    
    if (GFailureRate != GScrollFailureRate->GetValue()) {
        GFailureRate = GScrollFailureRate->GetValue();
        GDoRestart = true;
        stringstream str;
        str << "Failure Rate: " << GFailureRate << " disks/day";
        GTextFailureRate->SetText(str.str());
    }
    
    if (GSpareDisksPerDc != GScrollSpareDisks->GetValue()) {
        GSpareDisksPerDc = GScrollSpareDisks->GetValue();
        GDoRestart = true;
        stringstream str;
        str << "Spare Disks per DC: " << GSpareDisksPerDc;
        GTextSpareDisks->SetText(str.str());
    }
    
    if (GWriteSpeed != GScrollWriteSpeed->GetValue()) {
        GWriteSpeed = GScrollWriteSpeed->GetValue();
        GDoRestart = true;
        stringstream str;
        str << "Write Speed: " << GWriteSpeed << " MB/s";
        GTextWriteSpeed->SetText(str.str());
    }
    
    stringstream stats;
    stats << "Simulations: " << GSims << "\n";
    stats << "Data Loss Probability: " << fixed << setprecision(6) 

          << GDataLossProb * 100.0 << "%";
    GTextStats->SetText(stats.str());
}

void UpdateModel() {
    if (GDoRestart) {
        GDoRestart = false;
        GSim.Reset();
        DataLossByDay.clear();
        TotalSimsByDay.clear();
        GSims = 0;
    }
    
    GSim.Reset();
    bool hadDataLoss = false;
    
    for (Si32 day = 0; day < 30; ++day) {
        TotalSimsByDay[day]++;
        

        for (Si32 hour = 0; hour < 24; ++hour) {
            GSim.SimulateHour();
            
            if (GSim.LostData > 0 && !hadDataLoss) {
                DataLossByDay[day]++;
                hadDataLoss = true;
                break;
            }
        }
        
        if (hadDataLoss) {
            for (Si32 remainingDay = day + 1; remainingDay < 30; ++remainingDay) {
                TotalSimsByDay[remainingDay]++;
                DataLossByDay[remainingDay]++;
            }
            break;
        }
    }
    GSims++;

    if (!TotalSimsByDay.empty()) {
        GDataLossProb = static_cast<double>(DataLossByDay[29]) / 
                       static_cast<double>(TotalSimsByDay[29]);
    }
}

void DrawModel() {
    std::map<Si64, Si64> probabilityByDay;
    
    Si32 totalLosses = 0;
    for (Si32 day = 0; day < 30; ++day) {
        if (TotalSimsByDay[day] > 0) {
            totalLosses += DataLossByDay[day];
            double probability = static_cast<double>(totalLosses) / 
                               static_cast<double>(TotalSimsByDay[day]);
            probabilityByDay[day] = static_cast<Si64>(probability * 1000000);
        }
    }
    
    DrawCumulative(probabilityByDay, Rgba(255, 0, 0),
                  Vec2Si32(kLeftMargin, kTopMargin), 
                  Vec2Si32(kGraphWidth, kGraphHeight),
                  "Data Loss Probability Distribution");
}

void EasyMain() {
    GTheme = std::make_shared<GuiTheme>();
    GTheme->Load("data/gui_theme.xml");
    GFont.Load("data/arctic_one_bmf.fnt");
    ResizeScreen(kScreenWidth, kScreenHeight);

    GuiFactory guiFactory;
    guiFactory.theme_ = GTheme;
    GGui = guiFactory.MakeTransparentPanel();


    GScrollDisksPerDc = guiFactory.MakeHorizontalScrollbar();
    GScrollDisksPerDc->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 100);
    GScrollDisksPerDc->OnScrollChange = UpdateText;
    GScrollDisksPerDc->SetWidth(300);
    GScrollDisksPerDc->SetMinValue(10);
    GScrollDisksPerDc->SetMaxValue(1000);
    GScrollDisksPerDc->SetValue(100);
    GGui->AddChild(GScrollDisksPerDc);

    GScrollDiskSize = guiFactory.MakeHorizontalScrollbar();
    GScrollDiskSize->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 200);
    GScrollDiskSize->OnScrollChange = UpdateText;
    GScrollDiskSize->SetWidth(300);
    GScrollDiskSize->SetMinValue(100);
    GScrollDiskSize->SetMaxValue(16384);
    GScrollDiskSize->SetValue(4096);
    GGui->AddChild(GScrollDiskSize);

    GScrollFailureRate = guiFactory.MakeHorizontalScrollbar();
    GScrollFailureRate->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 300);
    GScrollFailureRate->OnScrollChange = UpdateText;
    GScrollFailureRate->SetWidth(300);
    GScrollFailureRate->SetMinValue(1);
    GScrollFailureRate->SetMaxValue(100);
    GScrollFailureRate->SetValue(3);
    GGui->AddChild(GScrollFailureRate);

    GScrollSpareDisks = guiFactory.MakeHorizontalScrollbar();
    GScrollSpareDisks->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 400);
    GScrollSpareDisks->OnScrollChange = UpdateText;
    GScrollSpareDisks->SetWidth(300);
    GScrollSpareDisks->SetMinValue(1);
    GScrollSpareDisks->SetMaxValue(100);
    GScrollSpareDisks->SetValue(10);
    GGui->AddChild(GScrollSpareDisks);

    GScrollWriteSpeed = guiFactory.MakeHorizontalScrollbar();
    GScrollWriteSpeed->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 500);
    GScrollWriteSpeed->OnScrollChange = UpdateText;
    GScrollWriteSpeed->SetWidth(300);
    GScrollWriteSpeed->SetMinValue(50);    // 50 MB/s
    GScrollWriteSpeed->SetMaxValue(1000);  // 1000 MB/s
    GScrollWriteSpeed->SetValue(100);
    GGui->AddChild(GScrollWriteSpeed);

    GTextDisksPerDc = guiFactory.MakeText();
    GTextDisksPerDc->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 100 + 29);
    GTextDisksPerDc->SetText("Disks per DC: " + std::to_string(GDisksPerDc));
    GGui->AddChild(GTextDisksPerDc);

    GTextDiskSize = guiFactory.MakeText();
    GTextDiskSize->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 200 + 29);
    GTextDiskSize->SetText("Disk Size: " + std::to_string(GDiskSize) + " GB");
    GGui->AddChild(GTextDiskSize);

    GTextFailureRate = guiFactory.MakeText();
    GTextFailureRate->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 300 + 29);
    GTextFailureRate->SetText("Failure Rate: " + std::to_string(GFailureRate) + " disks/day");
    GGui->AddChild(GTextFailureRate);

    GTextSpareDisks = guiFactory.MakeText();
    GTextSpareDisks->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 400 + 29);
    GTextSpareDisks->SetText("Spare Disks per DC: " + std::to_string(GSpareDisksPerDc));
    GGui->AddChild(GTextSpareDisks);

    GTextWriteSpeed = guiFactory.MakeText();
    GTextWriteSpeed->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 500 + 29);
    GTextWriteSpeed->SetText("Write Speed: " + std::to_string(GWriteSpeed) + " MB/s");
    GGui->AddChild(GTextWriteSpeed);

    GTextStats = guiFactory.MakeText();
    GTextStats->SetPos(kLeftMargin, kTopMargin + kGraphHeight + 70);
    GTextStats->SetText("Simulations: 0\nData Loss Probability: 0.000000%");
    GGui->AddChild(GTextStats);


    UpdateText();

    while (!IsKeyDownward(kKeyEscape)) {
        Clear();
        UpdateModel();
        for (Si32 messageIndex = 0; messageIndex < InputMessageCount(); ++messageIndex) {
            GGui->ApplyInput(GetInputMessage(messageIndex), nullptr);
        }
        DrawModel();
        UpdateText();
        GGui->Draw(Vec2Si32(0, 0));
        ShowFrame();
    }
}

