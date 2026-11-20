#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

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

#define IDC_VIEW_DATE 301
#define IDC_LISTVIEW  401

struct Record {
    std::string date, name;
    double total, expense, swipe, qr, cash;
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

std::string GetToday() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << (tm.tm_year + 1900) << '-'
        << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << '-'
        << std::setw(2) << std::setfill('0') << tm.tm_mday;
    return oss.str();
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

// ---------- ListView Helpers ----------
void InitListView() {
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    const char* headers[] = {"Date", "Name", "Total", "Expense", "Swipe", "QR", "Cash"};
    int widths[] = {90, 130, 80, 80, 80, 80, 80};

    for (int i = 0; i < 7; i++) {
        col.iSubItem = i;
        col.pszText = (LPSTR)headers[i];
        col.cx = widths[i];
        SendMessageA(hListView, LVM_INSERTCOLUMNA, i, (LPARAM)&col);
    }
}

void PopulateList(const std::string& filterDate) {
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
    int row = 0;
    for (const auto& r : records) {
        if (!filterDate.empty() && r.date != filterDate) continue;

        LVITEMA item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = (LPSTR)r.date.c_str();
        SendMessageA(hListView, LVM_INSERTITEMA, 0, (LPARAM)&item);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        const std::string cols[] = {
            r.name,
            oss.str() + std::to_string(r.total),
            std::to_string(r.expense),
            std::to_string(r.swipe),
            std::to_string(r.qr),
            std::to_string(r.cash)
        };
        // Re-format properly
        std::ostringstream o1, o2, o3, o4, o5;
        o1 << std::fixed << std::setprecision(2) << r.total;
        o2 << std::fixed << std::setprecision(2) << r.expense;
        o3 << std::fixed << std::setprecision(2) << r.swipe;
        o4 << std::fixed << std::setprecision(2) << r.qr;
        o5 << std::fixed << std::setprecision(2) << r.cash;

        const std::string finalCols[] = {r.name, o1.str(), o2.str(), o3.str(), o4.str(), o5.str()};

        for (int c = 1; c < 7; c++) {
            LVITEMA sub = {0};
            sub.iItem = row;
            sub.iSubItem = c;
            sub.pszText = (LPSTR)finalCols[c-1].c_str();
            SendMessageA(hListView, LVM_SETITEMTEXTA, row, (LPARAM)&sub);
        }
        row++;
    }
}

// ---------- UI Creation Helpers ----------
HWND CreateLabel(HWND p, const char* t, int x, int y, int w, int h) {
    HWND hw = CreateWindowExA(0, "STATIC", t, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                              x, y, w, h, p, NULL, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hw;
}

HWND CreateEdit(HWND p, int x, int y, int w, int h, int id) {
    HWND hw = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                              x, y, w, h, p, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
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
    r.expense = GetDouble(hExp);
    r.swipe   = GetDouble(hSwipe);
    r.qr      = GetDouble(hQR);
    r.cash    = GetDouble(hCash);

    if (r.name.empty()) {
        MessageBoxA(hwnd, "Please enter a Name.", "Missing Info", MB_ICONWARNING);
        SetFocus(hName);
        return;
    }

    records.push_back(r);
    PopulateList(GetText(hViewDate));

    // Clear inputs
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
    std::string htmlFile = GetAppDir() + "\\print_report.html";
    std::ofstream out(htmlFile);
    out << R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Expense Report</title><style>
    @page { size: A4; margin: 1.5cm; }
    body { font-family: Arial, sans-serif; text-align: center; color: #222; }
    h2 { margin-bottom: 20px; }
    table { margin: 0 auto; border-collapse: collapse; width: 100%; max-width: 800px; }
    th, td { border: 1px solid #000; padding: 10px; text-align: center; }
    th { background-color: #e8e8e8; font-weight: bold; }
    </style></head><body>
    <h2>Daily Expense & Collection Report</h2>
    <table>
    <tr><th>Date</th><th>Name</th><th>Total</th><th>Expense</th><th>Swipe</th><th>QR</th><th>Cash</th></tr>)";
    for (const auto& r : records) {
        out << "<tr><td>" << r.date << "</td><td>" << r.name << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.total << "</td>"
            << "<td>" << r.expense << "</td><td>" << r.swipe << "</td>"
            << "<td>" << r.qr << "</td><td>" << r.cash << "</td></tr>";
    }
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

// ---------- Window Procedure ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Initialize common controls for ListView
            INITCOMMONCONTROLSEX icex = {0};
            icex.dwSize = sizeof(icex);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            hFont = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            int x = 20, y = 20, lblW = 70, editW = 110, editH = 26, gap = 10;

            // Row 1
            CreateLabel(hwnd, "Date:", x, y, lblW, editH);
            hDate = CreateEdit(hwnd, x + lblW, y, editW, editH, IDC_DATE);
            SetWindowTextA(hDate, GetToday().c_str());

            CreateLabel(hwnd, "Name:", x + lblW + editW + gap, y, lblW, editH);
            hName = CreateEdit(hwnd, x + 2*lblW + editW + gap, y, 180, editH, IDC_NAME);

            // Row 2 - numeric inputs
            y += 40;
            const char* labels[] = {"Total:", "Expense:", "Swipe:", "QR:", "Cash:"};
            int ids[] = {IDC_TOTAL, IDC_EXP, IDC_SWIPE, IDC_QR, IDC_CASH};
            HWND* handles[] = {&hTotal, &hExp, &hSwipe, &hQR, &hCash};

            for (int i = 0; i < 5; i++) {
                int cx = x + i * (lblW + editW + gap);
                CreateLabel(hwnd, labels[i], cx, y, lblW, editH);
                *(handles[i]) = CreateEdit(hwnd, cx + lblW, y, editW, editH, ids[i]);
            }

            // Buttons row
            y += 45;
            CreateBtn(hwnd, "Add Record",     x,       y, 130, 32, IDC_BTN_ADD);
            CreateBtn(hwnd, "Print A4",       x + 140, y, 130, 32, IDC_BTN_PRINT);
            CreateBtn(hwnd, "Backup Data",    x + 280, y, 130, 32, IDC_BTN_BACKUP);
            CreateBtn(hwnd, "Clear All",      x + 420, y, 130, 32, IDC_BTN_CLEAR);

            // Filter row
            y += 50;
            CreateLabel(hwnd, "Filter by Date:", x, y, 110, editH);
            hViewDate = CreateEdit(hwnd, x + 110, y, 140, editH, IDC_VIEW_DATE);
            CreateBtn(hwnd, "Filter",     x + 260, y, 90, editH, IDC_BTN_FILTER);
            CreateBtn(hwnd, "Show All",   x + 360, y, 90, editH, IDC_BTN_SHOWALL);

            // ListView
            y += 40;
            hListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                x, y, 840, 340, hwnd, (HMENU)(INT_PTR)IDC_LISTVIEW, NULL, NULL);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
            InitListView();
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_BTN_ADD)     DoAddRecord(hwnd);
            else if (id == IDC_BTN_PRINT)   DoPrintA4(hwnd);
            else if (id == IDC_BTN_BACKUP)  DoBackup(hwnd);
            else if (id == IDC_BTN_FILTER)  PopulateList(GetText(hViewDate));
            else if (id == IDC_BTN_SHOWALL) {
                SetWindowTextA(hViewDate, "");
                PopulateList("");
            }
            else if (id == IDC_BTN_CLEAR) {
                if (MessageBoxA(hwnd, "Clear ALL records from memory?", "Confirm",
                                MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    records.clear();
                    PopulateList(GetText(hViewDate));
                }
            }
            return 0;
        }
        case WM_SIZE: {
            // Resize ListView to fit window
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
    wc.lpszClassName = "ExpenseAppClass";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    int w = 900, h = 650;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExA(0, "ExpenseAppClass", "Expense Manager Pro",
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
