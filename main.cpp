#include "engine/easy.h"
#include <map>
#include <iomanip>

using namespace arctic;  //NOLINT

std::shared_ptr<GuiTheme> g_theme;
std::shared_ptr<Panel> g_gui;
Font g_font;

// GUI элементы
std::shared_ptr<Text> g_text_disks_per_dc;
std::shared_ptr<Text> g_text_disk_size;
std::shared_ptr<Text> g_text_failure_rate;
std::shared_ptr<Text> g_text_data_loss;
std::shared_ptr<Text> g_text_stats;

std::shared_ptr<Scrollbar> g_scroll_disks_per_dc;
std::shared_ptr<Scrollbar> g_scroll_disk_size;
std::shared_ptr<Scrollbar> g_scroll_failure_rate;

// Параметры симуляции
Si32 g_disks_per_dc = 100;    // Дисков в каждом DC
Si32 g_disk_size = 4096;      // GB
Si32 g_failure_rate = 3;      // failures/day
Ui64 g_sims = 0;
double g_data_loss_prob = 0.0;

bool g_do_restart = true;  // Добавить в глобальные переменные

const Si32 GRAPH_WIDTH = 1200;  // Увеличим с 800 до 1200
const Si32 GRAPH_HEIGHT = 500;
const Si32 SCREEN_WIDTH = 1920;
const Si32 SCREEN_HEIGHT = 1080;
const Si32 LEFT_MARGIN = (SCREEN_WIDTH - GRAPH_WIDTH) / 2;
const Si32 TOP_MARGIN = (SCREEN_HEIGHT - GRAPH_HEIGHT) / 2;

struct Disk {
    enum State { ACTIVE, INACTIVE, FAULTY };
    State state = ACTIVE;
    double inactive_time = 0;  // Когда диск перешел в INACTIVE
    double rebuild_time = 0;   // Сколько времени нужно на восстановление
    Si32 dc_id = 0;           // ID датацентра
    Si32 data_id = -1;        // ID данных, хранящихся на диске
};

struct DataCenter {
    std::vector<Disk> disks;
    std::vector<Si32> spare_disks;
};

struct Sim {
    std::vector<DataCenter> dcs;  // 3 датацентра
    double current_time = 0;      // Текущее время в часах
    Si32 lost_data = 0;          // Количество потерянных данных
    
    void Reset() {
        dcs.resize(3); // 3 датацентра
        current_time = 0;
        lost_data = 0;
        
        // Инициализация дисков в каждом DC
        for (Si32 dc = 0; dc < 3; ++dc) {
            dcs[dc].disks.resize(g_disks_per_dc);
            dcs[dc].spare_disks.resize(g_disks_per_dc / 10); // 10% запасных
            
            for (auto& disk : dcs[dc].disks) {
                disk.state = Disk::ACTIVE;
                disk.dc_id = dc;
            }
        }
        
        // Распределение данных по дискам (3 копии)
        Si32 data_id = 0;
        for (Si32 i = 0; i < g_disks_per_dc; ++i) {
            Si32 disk1 = i;
            Si32 disk2 = i;
            Si32 disk3 = i;
            
            dcs[0].disks[disk1].data_id = data_id;
            dcs[1].disks[disk2].data_id = data_id;
            dcs[2].disks[disk3].data_id = data_id;
            
            data_id++;
        }
    }

    void SimulateHour() {
        // Моделирование отказов
        double failure_prob = g_failure_rate / (24.0 * g_disks_per_dc * 3);
        
        for (auto& dc : dcs) {
            for (auto& disk : dc.disks) {
                if (disk.state == Disk::ACTIVE && Random32() % 1000000 < failure_prob * 1000000) {
                    disk.state = Disk::INACTIVE;
                    disk.inactive_time = current_time;
                }
                else if (disk.state == Disk::INACTIVE) {
                    // Проверка перехода в FAULTY
                    if (current_time >= disk.inactive_time + 1.0) { // 1 час в INACTIVE
                        disk.state = Disk::FAULTY;
                        HandleDiskFailure(disk);
                    }
                }
            }
        }
        
        current_time += 1.0; // +1 час
    }

    void HandleDiskFailure(Disk& failed_disk) {
        // Проверяем живые копии данных
        Si32 data_id = failed_disk.data_id;
        Si32 alive_copies = 0;
        
        for (const auto& dc : dcs) {
            for (const auto& disk : dc.disks) {
                if (disk.data_id == data_id && disk.state != Disk::FAULTY) {
                    alive_copies++;
                }
            }
        }
        
        // Если все копии потеряны
        if (alive_copies == 0) {
            lost_data++;
            return;
        }
        
        // Восстановление на запасной диск
        auto& dc = dcs[failed_disk.dc_id];
        if (!dc.spare_disks.empty()) {
            Si32 spare_idx = dc.spare_disks.back();
            dc.spare_disks.pop_back();
            
            Disk& new_disk = dc.disks[spare_idx];
            new_disk.state = Disk::INACTIVE;
            new_disk.data_id = failed_disk.data_id;
            // Время восстановления зависит от размера диска
            new_disk.rebuild_time = current_time + (g_disk_size / 100.0); // 100 GB/час
        }
    }
};

Sim g_sim;

// Изменим тип для хранения статистики: ключ - день, значение - количество симуляций с потерей данных
std::map<Si32, Si32> g_data_loss_by_day;
std::map<Si32, Si32> g_total_sims_by_day; // общее количество симуляций для каждого дня

void DrawCumulative(std::map<Si64, Si64> &n_by_value, Rgba col, Vec2Si32 pos, Vec2Si32 size, const char* title) {
    std::vector<Vec2D> points;
    points.reserve(n_by_value.size()+2);

    if (n_by_value.size()) {
        points.emplace_back(n_by_value.begin()->first, 0.0);
    }
    double total = 0.0;
    for (auto it = n_by_value.begin(); it != n_by_value.end(); ++it) {
        total += it->second;
        points.emplace_back(it->first, total);
    }
    
    if (points.size()) {
        Vec2D min_v = points[0];
        Vec2D max_v = points[0];
        if (total > 0.0) {
            for (auto it = points.begin(); it != points.end(); ++it) {
                it->y /= total;
                min_v.x = std::min(min_v.x, it->x);
                min_v.y = std::min(min_v.y, it->y);
                max_v.x = std::max(max_v.x, it->x);
                max_v.y = std::max(max_v.y, it->y);
            }
        }

        DrawRectangle(pos, pos+size, Rgba(32,32,32));

        Vec2D scale = Vec2D(double(size.x) / (max_v.x - min_v.x), size.y);
        Vec2D p0 = points[0];
        for (Si32 i = 0; i < points.size(); ++i) {
            Vec2D p1 = points[i];
            Rgba color(255, 255, 255);
            if (p1.y > 0) {
                color = col;
            }
            DrawLine(pos+Vec2Si32(scale.x * (p0.x - min_v.x), scale.y * p0.y),
                    pos+Vec2Si32(scale.x * (p1.x - min_v.x), scale.y * p1.y),
                    color);
            p0 = p1;
        }

        g_font.Draw(title, pos.x, pos.y + size.y + 30, kTextOriginBottom);

        {
            std::stringstream str;
            str << min_v.x;
            g_font.Draw(str.str().c_str(), pos.x, pos.y, kTextOriginTop);
        }
        {
            std::stringstream str;
            str << max_v.x;
            g_font.Draw(str.str().c_str(), pos.x + size.x, pos.y, kTextOriginTop);
        }

        Si32 mx = MouseX();
        Si32 vmx = mx - pos.x;
        if (vmx >= 0 && vmx < size.x) {
            Si64 day = vmx / scale.x + min_v.x;
            for (Si32 i = 0; i < points.size(); ++i) {
                if (points[i].x > day) {
                    Si32 ii = std::max(0, i-1);

                    DrawLine(pos+Vec2Si32(scale.x * (points[ii].x - min_v.x), scale.y * 0.0),
                            pos+Vec2Si32(scale.x * (points[ii].x - min_v.x), scale.y * 1.0),
                            Rgba(128, 128, 128));

                    std::stringstream str;
                    str << std::fixed << std::setprecision(6)
                        << "Day " << day 
                        << "\nProbability of data loss: " 
                        << (points[ii].y * 100.0) << "%";
                    g_font.Draw(str.str().c_str(),
                              pos.x + Si32(scale.x * (points[ii].x - min_v.x)),
                              pos.y + Si32(scale.y * points[ii].y),
                              kTextOriginTop);
                    break;
                }
            }
        }

        // Подписи осей
        g_font.Draw("Days", pos.x + size.x/2, pos.y - 20, kTextOriginTop);
        g_font.Draw("Probability", pos.x - 20, pos.y + size.y/2, kTextOriginTop);

        // При наведении мыши показываем вероятность
        if (vmx >= 0 && vmx < size.x) {
            Si64 day = vmx / scale.x + min_v.x;
            for (Si32 i = 0; i < points.size(); ++i) {
                if (points[i].x > day) {
                    Si32 ii = std::max(0, i-1);
                    
                    DrawLine(pos+Vec2Si32(scale.x * (points[ii].x - min_v.x), scale.y * 0.0),
                            pos+Vec2Si32(scale.x * (points[ii].x - min_v.x), scale.y * 1.0),
                            Rgba(128, 128, 128));

                    std::stringstream str;
                    str << std::fixed << std::setprecision(6)
                        << "Day " << day 
                        << "\nProbability of data loss: " 
                        << (points[ii].y * 100.0) << "%";
                    g_font.Draw(str.str().c_str(),
                              pos.x + Si32(scale.x * (points[ii].x - min_v.x)),
                              pos.y + Si32(scale.y * points[ii].y),
                              kTextOriginTop);
                    break;
                }
            }
        }
    }
}

void UpdateText() {
    using namespace std;
    
    if (g_disks_per_dc != g_scroll_disks_per_dc->GetValue()) {
        g_disks_per_dc = g_scroll_disks_per_dc->GetValue();
        g_do_restart = true;  // Добавить эту строку
        stringstream str;
        str << "Disks per DC: " << g_disks_per_dc;
        g_text_disks_per_dc->SetText(str.str());
    }
    
    if (g_disk_size != g_scroll_disk_size->GetValue()) {
        g_disk_size = g_scroll_disk_size->GetValue();
        g_do_restart = true;  // Добавить эту строку
        stringstream str;
        str << "Disk Size: " << g_disk_size << " GB";
        g_text_disk_size->SetText(str.str());
    }
    
    if (g_failure_rate != g_scroll_failure_rate->GetValue()) {
        g_failure_rate = g_scroll_failure_rate->GetValue();
        g_do_restart = true;  // Добавить эту строку
        stringstream str;
        str << "Failure Rate: " << g_failure_rate << " disks/day";
        g_text_failure_rate->SetText(str.str());
    }
    
    stringstream stats;
    stats << "Simulations: " << g_sims << "\n";
    stats << "Data Loss Probability: " << fixed << setprecision(6) 
          << g_data_loss_prob * 100.0 << "%";
    g_text_stats->SetText(stats.str());
}

void UpdateModel() {
    if (g_do_restart) {
        g_do_restart = false;
        g_sim.Reset();
        g_data_loss_by_day.clear();
        g_total_sims_by_day.clear();
        g_sims = 0;
    }
    
    double t0 = Time();
    while (Time() - t0 < 1.0/60.0) {
        g_sim.Reset();
        bool had_data_loss = false;
        
        // Симулируем 30 дней
        for (Si32 day = 0; day < 30; ++day) {
            for (Si32 hour = 0; hour < 24; ++hour) {
                g_sim.SimulateHour();
            }
            
            // Увеличиваем счетчик симуляций для текущего дня
            g_total_sims_by_day[day]++;
            
            // Если в этот день произошла потеря данных
            if (g_sim.lost_data > 0 && !had_data_loss) {
                g_data_loss_by_day[day]++;
                had_data_loss = true; // отмечаем, что уже была потеря данных
            }
        }
        g_sims++;
    }
}

void DrawModel() {
    std::map<Si64, Si64> probability_by_day;
    for (const auto& pair : g_data_loss_by_day) {
        Si32 day = pair.first;
        Si32 losses = pair.second;
        double prob = static_cast<double>(losses) / g_total_sims_by_day[day];
        probability_by_day[day] = static_cast<Si64>(prob * 1000000);
    }
    
    DrawCumulative(probability_by_day, Rgba(255, 0, 0),
                  Vec2Si32(LEFT_MARGIN, TOP_MARGIN), 
                  Vec2Si32(GRAPH_WIDTH, GRAPH_HEIGHT),
                  "Data Loss Probability Distribution");
}

void EasyMain() {
    g_theme = std::make_shared<GuiTheme>();
    g_theme->Load("data/gui_theme.xml");
    g_font.Load("data/arctic_one_bmf.fnt");
    ResizeScreen(1920, 1080);

    GuiFactory gf;
    gf.theme_ = g_theme;
    g_gui = gf.MakeTransparentPanel();

    g_scroll_disks_per_dc = gf.MakeHorizontalScrollbar();
    g_scroll_disks_per_dc->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 100);
    g_scroll_disks_per_dc->OnScrollChange = UpdateText;
    g_scroll_disks_per_dc->SetWidth(300);
    g_scroll_disks_per_dc->SetMinValue(10);
    g_scroll_disks_per_dc->SetMaxValue(1000);
    g_scroll_disks_per_dc->SetValue(100);
    g_gui->AddChild(g_scroll_disks_per_dc);

    g_scroll_disk_size = gf.MakeHorizontalScrollbar();
    g_scroll_disk_size->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 200);
    g_scroll_disk_size->OnScrollChange = UpdateText;
    g_scroll_disk_size->SetWidth(300);
    g_scroll_disk_size->SetMinValue(100);
    g_scroll_disk_size->SetMaxValue(16384);
    g_scroll_disk_size->SetValue(4096);
    g_gui->AddChild(g_scroll_disk_size);

    g_scroll_failure_rate = gf.MakeHorizontalScrollbar();
    g_scroll_failure_rate->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 300);
    g_scroll_failure_rate->OnScrollChange = UpdateText;
    g_scroll_failure_rate->SetWidth(300);
    g_scroll_failure_rate->SetMinValue(1);
    g_scroll_failure_rate->SetMaxValue(100);
    g_scroll_failure_rate->SetValue(3);
    g_gui->AddChild(g_scroll_failure_rate);

    g_text_disks_per_dc = gf.MakeText();
    g_text_disks_per_dc->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 100 + 29);
    g_text_disks_per_dc->SetText("Disks per DC: " + std::to_string(g_disks_per_dc));
    g_gui->AddChild(g_text_disks_per_dc);

    g_text_disk_size = gf.MakeText();
    g_text_disk_size->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 200 + 29);
    g_text_disk_size->SetText("Disk Size: " + std::to_string(g_disk_size) + " GB");
    g_gui->AddChild(g_text_disk_size);

    g_text_failure_rate = gf.MakeText();
    g_text_failure_rate->SetPos(LEFT_MARGIN - 320, TOP_MARGIN + 300 + 29);
    g_text_failure_rate->SetText("Failure Rate: " + std::to_string(g_failure_rate) + " disks/day");
    g_gui->AddChild(g_text_failure_rate);

    g_text_stats = gf.MakeText();
    g_text_stats->SetPos(LEFT_MARGIN, TOP_MARGIN + GRAPH_HEIGHT + 20);
    g_text_stats->SetText("Simulations: 0\nData Loss Probability: 0.000000%");
    g_gui->AddChild(g_text_stats);

    UpdateText();

    while (!IsKeyDownward(kKeyEscape)) {
        Clear();
        UpdateModel();
        for (Si32 i = 0; i < InputMessageCount(); ++i) {
            g_gui->ApplyInput(GetInputMessage(i), nullptr);
        }
        DrawModel();
        g_gui->Draw(Vec2Si32(0, 0));
        ShowFrame();
    }
}
