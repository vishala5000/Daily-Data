#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

// Fix for missing Windows constant
#define EM_SETBKGNDCOLOR (WM_USER + 1)

// Control IDs
#define IDC_DATE     101
#define IDC_NAME     102
#define IDC_TOTAL    103
#define IDC_EXP      104
#define IDC_SWIPE    105
#define IDC_QR       106
#define IDC_CASH     107

#define IDC_BTN_ADD      201
#define IDC_BTN_PRINT    202
#define IDC_BTN_BACKUP   203
#define IDC_BTN_FILTER   204
#define IDC_BTN_SHOWALL  205
#define IDC_BTN_CLEAR    206
#define IDC_BTN_RESTORE  207
#define IDC_BTN_TODAY    208
#define IDC_BTN_YESTERDAY 209
#define IDC_BTN_FTODAY   210
#define IDC_BTN_FYESTERDAY 211

#define IDC_VIEW_DATE 301
#define IDC_LISTVIEW  401

struct Record {
    std::string date, name, expense;
    double total, swipe, qr, cash;
};

std::vector<Record> records;
HWND hDate, hName, hTotal, hExp, hSwipe, hQR, hCash;
HWND hViewDate, hListView;
HFONT hFont;

// ---------- Utility Functions ----------
std::string GetAppDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : fullPath.substr(0, pos);
}

// Format date as DD/MM/YYYY
std::string GetFormattedDate(std::time_t time) {
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << tm.tm_mday << '/'
        << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << '/'
        << (tm.tm_year + 1900);
    return oss.str();
}

std::string GetToday() {
    return GetFormattedDate(std::time(nullptr));
}

std::string GetYesterday() {
    return GetFormattedDate(std::time(nullptr) - 86400);
}

// Convert DD/MM/YYYY to YYYYMMDD integer for comparison
int DateToInt(const std::string& dateStr) {
    if (dateStr.length() < 10) return 0;
    
    int day = 0, month = 0, year = 0;
    
    // Try DD/MM/YYYY format
    if (dateStr[2] == '/' || dateStr[2] == '-') {
        try {
            day = std::stoi(dateStr.substr(0, 2));
            month = std::stoi(dateStr.substr(3, 2));
            year = std::stoi(dateStr.substr(6, 4));
        } catch (...) { return 0; }
    } 
    // Try YYYY-MM-DD format (for backward compatibility)
    else if (dateStr[4] == '-' || dateStr[4] == '/') {
        try {
            year = std::stoi(dateStr.substr(0, 4));
            month = std::stoi(dateStr.substr(5, 2));
            day = std::stoi(dateStr.substr(8, 2));
        } catch (...) { return 0; }
    } else {
        return 0;
    }
    
    return year * 10000 + month * 100 + day;
}

std::string GetText(HWND hwnd) {
    int len = GetWindowTextLengthA(hwnd);
    if (len == 0) return "";
    std::string str(len, '\0');
    GetWindowTextA(hwnd, &str[0], len + 1);
    return str;
}

double GetDouble(HWND hwnd) {
    std::string text = GetText(hwnd);
    if (text.empty()) return 0.0;
    try { return std::stod(text); } catch (...) { return 0.0; }
}

// Extract number from text like "Refund 500" -> 500
double ExtractNumber(const std::string& text) {
    if (text.empty()) return 0.0;
    int i = (int)text.length() - 1;
    while (i >= 0 && !isdigit(text[i]) && text[i] != '.') i--;
    if (i < 0) return 0.0;
    int end = i + 1;
    while (i >= 0 && (isdigit(text[i]) || text[i] == '.')) i--;
    int start = i + 1;
    try {
        return std::stod(text.substr(start, end - start));
    } catch (...) {
        return 0.0;
    }
}

// Auto-calculate Cash
void RecalculateCash() {
    double total = GetDouble(hTotal);
    double expense = ExtractNumber(GetText(hExp));
    double swipe = GetDouble(hSwipe);
    double qr = GetDouble(hQR);
    double cash = total - expense - swipe - qr;
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", cash);
    SetWindowTextA(hCash, buf);
}

// ---------- Auto-Save and Auto-Load ----------
void SaveData() {
    std::string saveFile = GetAppDir() + "\\daily_cash_data.dat";
    std::ofstream out(saveFile);
    if (!out.is_open()) return;
    
    for (const auto& r : records) {
        out << r.date << "|" << r.name << "|" << r.total << "|"
            << r.expense << "|" << r.swipe << "|" << r.qr << "|" << r.cash << "\n";
    }
    out.close();
}

bool LoadData() {
    std::string saveFile = GetAppDir() + "\\daily_cash_data.dat";
    std::ifstream in(saveFile);
    if (!in.is_open()) return false;
    
    records.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 7) {
            Record r;
            r.date = tokens[0];
            r.name = tokens[1];
            try {
                r.total = std::stod(tokens[2]);
                r.expense = tokens[3];
                r.swipe = std::stod(tokens[4]);
                r.qr = std::stod(tokens[5]);
                r.cash = std::stod(tokens[6]);
                records.push_back(r);
            } catch (...) {
                continue;
            }
        }
    }
    in.close();
    return true;
}

bool LoadFromFile(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    
    std::vector<Record> tempRecords;
    std::string line;
    bool firstLine = true;
    
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        
        if (firstLine && (line.find("Date,") == 0 || line.find("Date|") == 0)) {
            firstLine = false;
            continue;
        }
        firstLine = false;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() < 7) {
            iss.clear();
            iss.str(line);
            tokens.clear();
            while (std::getline(iss, token, ',')) {
                tokens.push_back(token);
            }
        }
        
        if (tokens.size() >= 7) {
            Record r;
            r.date = tokens[0];
            r.name = tokens[1];
            try {
                r.total = std::stod(tokens[2]);
                r.expense = tokens[3];
                r.swipe = std::stod(tokens[4]);
                r.qr = std::stod(tokens[5]);
                r.cash = std::stod(tokens[6]);
                tempRecords.push_back(r);
            } catch (...) {
                continue;
            }
        }
    }
    in.close();
    
    if (!tempRecords.empty()) {
        records = tempRecords;
        return true;
    }
    return false;
}

// ---------- ListView Helpers ----------
void InitListView() {
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    const char* headers[] = {"Date", "Name", "Total", "Expense", "Swipe", "QR", "Cash"};
    int widths[] = {100, 130, 80, 100, 80, 80, 80};

    for (int i = 0; i < 7; i++) {
        col.iSubItem = i;
        col.pszText = (LPSTR)headers[i];
        col.cx = widths[i];
        SendMessageA(hListView, LVM_INSERTCOLUMNA, i, (LPARAM)&col);
    }
}

// Cumulative filter: show all records with date <= filterDate, sorted newest first
void PopulateList(const std::string& filterDate) {
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
    
    // Filter records
    std::vector<const Record*> filtered;
    int filterInt = DateToInt(filterDate);
    
    for (const auto& r : records) {
        int recordInt = DateToInt(r.date);
        if (filterDate.empty() || recordInt <= filterInt) {
            filtered.push_back(&r);
        }
    }
    
    // Sort by date descending (newest first)
    std::sort(filtered.begin(), filtered.end(), [](const Record* a, const Record* b) {
        return DateToInt(a->date) > DateToInt(b->date);
    });
    
    int row = 0;
    double sumTotal = 0, sumExpense = 0, sumSwipe = 0, sumQR = 0, sumCash = 0;
    
    for (const auto* rPtr : filtered) {
        const Record& r = *rPtr;
        
        LVITEMA item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = (LPSTR)r.date.c_str();
        SendMessageA(hListView, LVM_INSERTITEMA, 0, (LPARAM)&item);

        std::ostringstream o1, o2, o3, o4, o5;
        o1 << std::fixed << std::setprecision(2) << r.total;
        o2 << std::fixed << std::setprecision(2) << ExtractNumber(r.expense);
        o3 << std::fixed << std::setprecision(2) << r.swipe;
        o4 << std::fixed << std::setprecision(2) << r.qr;
        o5 << std::fixed << std::setprecision(2) << r.cash;

        LVITEMA sub = {0};
        sub.iItem = row;
        sub.iSubItem = 1;
        sub.pszText = (LPSTR)r.name.c_str();
        SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        
        sub.iSubItem = 3;
        sub.pszText = (LPSTR)r.expense.c_str();
        SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        
        const std::string numCols[] = {o1.str(), o2.str(), o3.str(), o4.str(), o5.str()};
        for (int c = 0; c < 5; c++) {
            int colIdx = (c == 0) ? 2 : (c + 2);
            sub.iSubItem = colIdx;
            sub.pszText = (LPSTR)numCols[c].c_str();
            SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        }
        
        sumTotal += r.total;
        sumExpense += ExtractNumber(r.expense);
        sumSwipe += r.swipe;
        sumQR += r.qr;
        sumCash += r.cash;
        
        row++;
    }
    
    // Add Total row
    if (row > 0) {
        LVITEMA item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = (LPSTR)"";
        SendMessageA(hListView, LVM_INSERTITEMA, 0, (LPARAM)&item);
        
        std::ostringstream o1, o2, o3, o4, o5;
        o1 << std::fixed << std::setprecision(2) << sumTotal;
        o2 << std::fixed << std::setprecision(2) << sumExpense;
        o3 << std::fixed << std::setprecision(2) << sumSwipe;
        o4 << std::fixed << std::setprecision(2) << sumQR;
        o5 << std::fixed << std::setprecision(2) << sumCash;
        
        LVITEMA sub = {0};
        sub.iItem = row;
        
        sub.iSubItem = 1;
        sub.pszText = (LPSTR)"Total";
        SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        
        const std::string totals[] = {o1.str(), o2.str(), o3.str(), o4.str(), o5.str()};
        for (int c = 0; c < 5; c++) {
            sub.iSubItem = c + 2;
            sub.pszText = (LPSTR)totals[c].c_str();
            SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        }
    }
}

// ---------- UI Creation Helpers ----------
HWND CreateLabel(HWND p, const char* t, int x, int y, int w, int h) {
    HWND hw = CreateWindowExA(0, "STATIC", t, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                              x, y, w, h, p, NULL, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hw;
}

HWND CreateEdit(HWND p, int x, int y, int w, int h, int id, bool readOnly = false) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    if (readOnly) style |= ES_READONLY;
    HWND hw = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", style,
                              x, y, w, h, p, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (readOnly) {
        SendMessage(hw, EM_SETBKGNDCOLOR, 0, RGB(240, 240, 240));
    }
    return hw;
}

HWND CreateBtn(HWND p, const char* t, int x, int y, int w, int h, int id) {
    HWND hw = CreateWindowExA(0, "BUTTON", t,
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                              x, y, w, h, p, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hw;
}

// ---------- Action Handlers ----------
void DoAddRecord(HWND hwnd) {
    Record r;
    r.date = GetText(hDate);
    if (r.date.empty()) r.date = GetToday();
    r.name = GetText(hName);
    r.total   = GetDouble(hTotal);
    r.expense = GetText(hExp);
    r.swipe   = GetDouble(hSwipe);
    r.qr      = GetDouble(hQR);
    r.cash    = r.total - ExtractNumber(r.expense) - r.swipe - r.qr;

    if (r.name.empty()) {
        MessageBoxA(hwnd, "Please enter a Name.", "Missing Info", MB_ICONWARNING);
        SetFocus(hName);
        return;
    }

    records.push_back(r);
    SaveData();
    PopulateList(GetText(hViewDate));

    SetWindowTextA(hName, "");
    SetWindowTextA(hTotal, "");
    SetWindowTextA(hExp, "");
    SetWindowTextA(hSwipe, "");
    SetWindowTextA(hQR, "");
    SetWindowTextA(hCash, "");
    SetFocus(hName);
}

void DoPrintA4(HWND hwnd) {
    if (records.empty()) {
        MessageBoxA(hwnd, "No data to print.", "Info", MB_ICONINFORMATION);
        return;
    }
    
    std::string filterDate = GetText(hViewDate);
    std::string htmlFile = GetAppDir() + "\\print_report.html";
    std::ofstream out(htmlFile);
    
    // Filter and sort records
    std::vector<const Record*> filtered;
    int filterInt = DateToInt(filterDate);
    
    for (const auto& r : records) {
        int recordInt = DateToInt(r.date);
        if (filterDate.empty() || recordInt <= filterInt) {
            filtered.push_back(&r);
        }
    }
    
    std::sort(filtered.begin(), filtered.end(), [](const Record* a, const Record* b) {
        return DateToInt(a->date) > DateToInt(b->date);
    });
    
    out << R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Daily Cash Entry</title><style>
    @page { size: A4; margin: 1.5cm; }
    body { font-family: Arial, sans-serif; color: #222; }
    table { margin: 0 auto; border-collapse: collapse; width: 100%; max-width: 800px; }
    th, td { border: 1px solid #000; padding: 10px; text-align: center; }
    th { background-color: #e8e8e8; font-weight: bold; }
    .total-row { font-weight: bold; background-color: #f0f0f0; }
    </style></head><body>
    <table>
    <tr><th>Date</th><th>Name</th><th>Total</th><th>Expense</th><th>Swipe</th><th>QR</th><th>Cash</th></tr>)";
    
    double sumTotal = 0, sumExpense = 0, sumSwipe = 0, sumQR = 0, sumCash = 0;
    
    for (const auto* rPtr : filtered) {
        const Record& r = *rPtr;
        out << "<tr><td>" << r.date << "</td><td>" << r.name << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.total << "</td>"
            << "<td>" << r.expense << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.swipe << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.qr << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.cash << "</td></tr>";
        
        sumTotal += r.total;
        sumExpense += ExtractNumber(r.expense);
        sumSwipe += r.swipe;
        sumQR += r.qr;
        sumCash += r.cash;
    }
    
    out << "<tr class=\"total-row\"><td></td><td>Total</td>"
        << "<td>" << std::fixed << std::setprecision(2) << sumTotal << "</td>"
        << "<td>" << std::fixed << std::setprecision(2) << sumExpense << "</td>"
        << "<td>" << std::fixed << std::setprecision(2) << sumSwipe << "</td>"
        << "<td>" << std::fixed << std::setprecision(2) << sumQR << "</td>"
        << "<td>" << std::fixed << std::setprecision(2) << sumCash << "</td></tr>";
    
    out << "</table><script>window.onload=function(){window.print();}</script></body></html>";
    out.close();
    ShellExecuteA(hwnd, "open", htmlFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void DoBackup(HWND hwnd) {
    if (records.empty()) {
        MessageBoxA(hwnd, "No data to backup.", "Info", MB_ICONINFORMATION);
        return;
    }
    std::string csvFile = GetAppDir() + "\\data_backup.csv";
    std::ofstream out(csvFile);
    out << "Date,Name,Total,Expense,Swipe,QR,Cash\n";
    for (const auto& r : records) {
        out << r.date << "," << r.name << "," << r.total << ","
            << r.expense << "," << r.swipe << "," << r.qr << "," << r.cash << "\n";
    }
    out.close();
    MessageBoxA(hwnd, "Backup saved successfully in the app folder!", "Success", MB_ICONINFORMATION);
}

void DoRestoreBackup(HWND hwnd) {
    char fileName[MAX_PATH] = "";
    
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Data Files (*.dat;*.csv)\0*.dat;*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = "Select Backup File to Restore";
    
    if (GetOpenFileNameA(&ofn)) {
        if (LoadFromFile(fileName)) {
            SaveData();
            PopulateList(GetText(hViewDate));
            MessageBoxA(hwnd, "Backup restored successfully!", "Success", MB_ICONINFORMATION);
        } else {
            MessageBoxA(hwnd, "Failed to load backup file.", "Error", MB_ICONERROR);
        }
    }
}

// ---------- Window Procedure ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex = {0};
            icex.dwSize = sizeof(icex);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            hFont = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            int x = 20, y = 20, lblW = 70, editW = 110, editH = 26, gap = 10;

            // Row 1: Date with Today/Yesterday buttons
            CreateLabel(hwnd, "Date:", x, y, lblW, editH);
            hDate = CreateEdit(hwnd, x + lblW, y, editW, editH, IDC_DATE);
            SetWindowTextA(hDate, GetToday().c_str());
            
            CreateBtn(hwnd, "Today",     x + lblW + editW + 5,  y, 60, editH, IDC_BTN_TODAY);
            CreateBtn(hwnd, "Yesterday", x + lblW + editW + 70, y, 80, editH, IDC_BTN_YESTERDAY);

            CreateLabel(hwnd, "Name:", x + lblW + editW + 160, y, lblW, editH);
            hName = CreateEdit(hwnd, x + 2*lblW + editW + 160, y, 180, editH, IDC_NAME);

            // Row 2
            y += 40;
            const char* labels[] = {"Total:", "Expense:", "Swipe:", "QR:", "Cash:"};
            int ids[] = {IDC_TOTAL, IDC_EXP, IDC_SWIPE, IDC_QR, IDC_CASH};
            HWND* handles[] = {&hTotal, &hExp, &hSwipe, &hQR, &hCash};

            for (int i = 0; i < 5; i++) {
                int cx = x + i * (lblW + editW + gap);
                CreateLabel(hwnd, labels[i], cx, y, lblW, editH);
                bool readOnly = (i == 4);
                *(handles[i]) = CreateEdit(hwnd, cx + lblW, y, editW, editH, ids[i], readOnly);
            }

            // Buttons row
            y += 45;
            int btnW = 120;
            CreateBtn(hwnd, "Add Record",     x,              y, btnW, 32, IDC_BTN_ADD);
            CreateBtn(hwnd, "Print A4",       x + btnW + 10,  y, btnW, 32, IDC_BTN_PRINT);
            CreateBtn(hwnd, "Backup Data",    x + 2*(btnW+10), y, btnW, 32, IDC_BTN_BACKUP);
            CreateBtn(hwnd, "Restore Backup", x + 3*(btnW+10), y, btnW, 32, IDC_BTN_RESTORE);
            CreateBtn(hwnd, "Clear All",      x + 4*(btnW+10), y, btnW, 32, IDC_BTN_CLEAR);

            // Filter row with Today/Yesterday buttons
            y += 50;
            CreateLabel(hwnd, "View up to Date:", x, y, 120, editH);
            hViewDate = CreateEdit(hwnd, x + 120, y, 120, editH, IDC_VIEW_DATE);
            CreateBtn(hwnd, "Today",     x + 250, y, 60, editH, IDC_BTN_FTODAY);
            CreateBtn(hwnd, "Yesterday", x + 315, y, 80, editH, IDC_BTN_FYESTERDAY);
            CreateBtn(hwnd, "Show View", x + 405, y, 90, editH, IDC_BTN_FILTER);
            CreateBtn(hwnd, "Show All",  x + 505, y, 90, editH, IDC_BTN_SHOWALL);

            // ListView
            y += 40;
            hListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                x, y, 840, 340, hwnd, (HMENU)(INT_PTR)IDC_LISTVIEW, NULL, NULL);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
            InitListView();
            
            LoadData();
            PopulateList("");
            
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            
            if (code == EN_CHANGE && (id == IDC_TOTAL || id == IDC_EXP || id == IDC_SWIPE || id == IDC_QR)) {
                RecalculateCash();
            }
            
            if (id == IDC_BTN_ADD)     DoAddRecord(hwnd);
            else if (id == IDC_BTN_PRINT)   DoPrintA4(hwnd);
            else if (id == IDC_BTN_BACKUP)  DoBackup(hwnd);
            else if (id == IDC_BTN_RESTORE) DoRestoreBackup(hwnd);
            else if (id == IDC_BTN_TODAY) {
                SetWindowTextA(hDate, GetToday().c_str());
            }
            else if (id == IDC_BTN_YESTERDAY) {
                SetWindowTextA(hDate, GetYesterday().c_str());
            }
            else if (id == IDC_BTN_FTODAY) {
                SetWindowTextA(hViewDate, GetToday().c_str());
                PopulateList(GetToday());
            }
            else if (id == IDC_BTN_FYESTERDAY) {
                SetWindowTextA(hViewDate, GetYesterday().c_str());
                PopulateList(GetYesterday());
            }
            else if (id == IDC_BTN_FILTER)  {
                PopulateList(GetText(hViewDate));
            }
            else if (id == IDC_BTN_SHOWALL) {
                SetWindowTextA(hViewDate, "");
                PopulateList("");
            }
            else if (id == IDC_BTN_CLEAR) {
                if (MessageBoxA(hwnd, "Clear ALL records from memory?", "Confirm",
                                MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    records.clear();
                    SaveData();
                    PopulateList(GetText(hViewDate));
                }
            }
            return 0;
        }
        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            MoveWindow(hListView, 20, 230, rc.right - 40, rc.bottom - 250, TRUE);
            return 0;
        }
        case WM_DESTROY:
            DeleteObject(hFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ---------- Entry Point ----------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DailyCashEntryClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    int w = 900, h = 650;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExA(0, "DailyCashEntryClass", "Daily Cash Entry",
        WS_OVERLAPPEDWINDOW,
        (sw - w) / 2, (sh - h) / 2, w, h,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
