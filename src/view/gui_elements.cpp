#include "gui_elements.h"
#include <sstream>
#include <iomanip>
#include "../utils/logger.h"

namespace arctic {

Font GFont; 

const Ui32 kGraphWidth = 1200;
const Ui32 kGraphHeight = 500;
const Ui32 kScreenWidth = 1920;
const Ui32 kScreenHeight = 1080;
const Ui32 kLeftMargin = (kScreenWidth - kGraphWidth) / 2;
const Ui32 kTopMargin = (kScreenHeight - kGraphHeight) / 2;
const Ui32 kControlsYOffset = -70;

void InitializeGui(GuiElements& gui) {
    LOG("Initializing GUI");
    gui.Theme = std::make_shared<GuiTheme>();
    gui.Theme->Load("data/gui_theme.xml");
    gui.Font.Load("data/arctic_one_bmf.fnt");

    GuiFactory guiFactory;
    guiFactory.theme_ = gui.Theme;
    gui.Gui = guiFactory.MakeTransparentPanel();

    gui.ScrollDiskSize = guiFactory.MakeHorizontalScrollbar();
    gui.ScrollDiskSize->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 200);
    gui.ScrollDiskSize->SetWidth(300);
    gui.ScrollDiskSize->SetMinValue(100);
    gui.ScrollDiskSize->SetMaxValue(16384);
    gui.ScrollDiskSize->SetValue(4096);
    gui.Gui->AddChild(gui.ScrollDiskSize);

    gui.ScrollFailureRate = guiFactory.MakeHorizontalScrollbar();
    gui.ScrollFailureRate->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 300);
    gui.ScrollFailureRate->SetWidth(300);
    gui.ScrollFailureRate->SetMinValue(1);
    gui.ScrollFailureRate->SetMaxValue(100);
    gui.ScrollFailureRate->SetValue(3);
    gui.Gui->AddChild(gui.ScrollFailureRate);

    gui.ScrollSpareDisks = guiFactory.MakeHorizontalScrollbar();
    gui.ScrollSpareDisks->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 400);
    gui.ScrollSpareDisks->SetWidth(300);
    gui.ScrollSpareDisks->SetMinValue(1);
    gui.ScrollSpareDisks->SetMaxValue(100);
    gui.ScrollSpareDisks->SetValue(10);
    gui.Gui->AddChild(gui.ScrollSpareDisks);

    gui.ScrollWriteSpeed = guiFactory.MakeHorizontalScrollbar();
    gui.ScrollWriteSpeed->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 500);
    gui.ScrollWriteSpeed->SetWidth(300);
    gui.ScrollWriteSpeed->SetMinValue(50);
    gui.ScrollWriteSpeed->SetMaxValue(1000);
    gui.ScrollWriteSpeed->SetValue(100);
    gui.Gui->AddChild(gui.ScrollWriteSpeed);

    gui.ScrollDisksPerDc = guiFactory.MakeHorizontalScrollbar();
    gui.ScrollDisksPerDc->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 100);
    gui.ScrollDisksPerDc->SetWidth(300);
    gui.ScrollDisksPerDc->SetMinValue(10);
    gui.ScrollDisksPerDc->SetMaxValue(1000);
    gui.ScrollDisksPerDc->SetValue(100);
    gui.Gui->AddChild(gui.ScrollDisksPerDc);

    gui.TextDiskSize = guiFactory.MakeText();
    gui.TextDiskSize->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 200 + 29);
    gui.TextDiskSize->SetText("Disk Size: 4096 GB");
    gui.Gui->AddChild(gui.TextDiskSize);

    gui.TextFailureRate = guiFactory.MakeText();
    gui.TextFailureRate->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 300 + 29);
    gui.TextFailureRate->SetText("Failure Rate: 3 disks/day");
    gui.Gui->AddChild(gui.TextFailureRate);

    gui.TextSpareDisks = guiFactory.MakeText();
    gui.TextSpareDisks->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 400 + 29);
    gui.TextSpareDisks->SetText("Spare Disks per DC: 10");
    gui.Gui->AddChild(gui.TextSpareDisks);

    gui.TextWriteSpeed = guiFactory.MakeText();
    gui.TextWriteSpeed->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 500 + 29);
    gui.TextWriteSpeed->SetText("Write Speed: 100 MB/s");
    gui.Gui->AddChild(gui.TextWriteSpeed);

    gui.TextDisksPerDc = guiFactory.MakeText();
    gui.TextDisksPerDc->SetPos(kLeftMargin - 320, kTopMargin + kControlsYOffset + 100 + 29);
    gui.TextDisksPerDc->SetText("Disks per DC: 100");
    gui.Gui->AddChild(gui.TextDisksPerDc);

    gui.TextStats = guiFactory.MakeText();
    gui.TextStats->SetPos(kLeftMargin, kTopMargin + kGraphHeight + 70);
    gui.TextStats->SetText("Simulations: 0\nData Loss Probability: 0.000000%");
    gui.Gui->AddChild(gui.TextStats);

    gui.TextDataLossTitle = guiFactory.MakeText();
    gui.TextDataLossTitle->SetPos(kLeftMargin, kTopMargin + 520);
    gui.TextDataLossTitle->SetText("Data Loss Probability Distribution");
    gui.Gui->AddChild(gui.TextDataLossTitle);

    LOG("GUI initialized");
    LOG("GUI elements created");
}

void UpdateGuiText(GuiElements& gui) {
    using namespace std;

    stringstream str;
    str << "Simulations: " << GSims << "\n";
    str << "Data Loss Probability: " << fixed << setprecision(6) 
        << GDataLossProb * 100.0 << "%";
    gui.TextStats->SetText(str.str());

}

void DrawCumulative(const std::map<Si64, Si64>& valuesByCount, Rgba color, 
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

        DrawLabelsAndMouseInteraction(points, position, size, scale, title);
    }
}

void DrawLabelsAndMouseInteraction(const std::vector<Vec2D>& points, 
                                 Vec2Si32 position, Vec2Si32 size, 
                                 Vec2D scale, const char* title) {
    GFont.Draw("Days", position.x + size.x / 2, position.y - 20, kTextOriginTop);
    GFont.Draw("Probability", position.x - 20, position.y + size.y / 2, kTextOriginTop);
    GFont.Draw("0", position.x, position.y, kTextOriginTop);
    GFont.Draw("30", position.x + size.x, position.y, kTextOriginTop);

    HandleMouseInteraction(points, position, size, scale);
}

void DrawSimulation(const std::map<Si64, Si64>& probabilityByDay) {
    DrawCumulative(probabilityByDay, Rgba(255, 0, 0),
                  Vec2Si32(kLeftMargin, kTopMargin), 
                  Vec2Si32(kGraphWidth, kGraphHeight),
                  "Data Loss Probability Distribution");
}

void HandleMouseInteraction(const std::vector<Vec2D>& points, 
                          Vec2Si32 position, Vec2Si32 size, 
                          Vec2D scale) {
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

} // namespace arctic 