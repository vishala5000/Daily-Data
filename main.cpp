#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <limits>
#include <windows.h>

// UI Color Codes for Windows Console
// 11 = Light Cyan, 14 = Light Yellow, 10 = Light Green, 12 = Light Red, 15 = White, 8 = Dark Gray

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void SetColor(WORD color) {
    SetConsoleTextAttribute(hConsole, color);
}

void ClearScreen() {
    system("cls");
}

void InitConsole() {
    SetConsoleTitleA("Expense Manager Pro");
    system("mode con cols=95 lines=40");
    // 1F = Blue Background, Bright White Text. Gives a modern dark-theme look.
    system("color 1F"); 
    SetConsoleOutputCP(65001); 
}

void PrintHeader() {
    SetColor(11); 
    std::cout << "\n";
    std::cout << "  =====================================================================\n";
    std::cout << "  ||                                                                 ||\n";
    std::cout << "  ||                 $$  EXPENSE MANAGER PRO  $$                     ||\n";
    std::cout << "  ||                                                                 ||\n";
    std::cout << "  =====================================================================\n";
    SetColor(15); 
}

void PrintMenu() {
    ClearScreen();
    PrintHeader();
    
    SetColor(14); 
    std::cout << "\n  [1] [+] Add Data (Endless Entry)\n";
    std::cout << "  [2] [P] Print Data (A4 Size Perfectly Aligned)\n";
    std::cout << "  [3] [B] Backup All Data (Same Directory)\n";
    std::cout << "  [4] [V] View Data (Date-wise)\n";
    std::cout << "  [5] [X] Exit\n";
    
    SetColor(8); 
    std::cout << "\n  ---------------------------------------------------------------------\n";
    
    SetColor(15); 
    std::cout << "  >>> Enter your choice (1-5): ";
}

void PrintSuccess(const std::string& msg) {
    SetColor(10); 
    std::cout << "\n  [SUCCESS] " << msg << "\n";
    SetColor(15);
}

void PrintError(const std::string& msg) {
    SetColor(12); 
    std::cout << "\n  [ERROR] " << msg << "\n";
    SetColor(15);
}

void PrintInfo(const std::string& msg) {
    SetColor(11); 
    std::cout << "\n  [INFO] " << msg << "\n";
    SetColor(15);
}

void Pause() {
    SetColor(8);
    std::cout << "\n  Press Enter to return to main menu...";
    SetColor(15);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

struct Record {
    std::string date;
    std::string name;
    double total;
    double expense;
    double swipe;
    double qr;
    double cash;
};

std::string getFormattedDate(std::time_t time) {
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << (tm.tm_year + 1900) << '-'
        << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1) << '-'
        << std::setw(2) << std::setfill('0') << tm.tm_mday;
    return oss.str();
}

std::string getToday() { return getFormattedDate(std::time(nullptr)); }
std::string getYesterday() { return getFormattedDate(std::time(nullptr) - 86400); }

std::string getAppDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : fullPath.substr(0, pos);
}

void addRecords(std::vector<Record>& records) {
    ClearScreen();
    PrintHeader();
    SetColor(14);
    std::cout << "\n  --- [+] ADD NEW DATA ---\n";
    SetColor(15);
    
    bool keepAdding = true;
    while (keepAdding) {
        Record r;
        SetColor(11);
        std::cout << "\n  > Enter Date (YYYY-MM-DD) [Press Enter for Today]: ";
        SetColor(15);
        std::getline(std::cin, r.date);
        if (r.date.empty()) r.date = getToday();

        SetColor(11);
        std::cout << "  > Enter Name: ";
        SetColor(15);
        std::getline(std::cin, r.name);

        auto getDouble = [](const std::string& prompt) {
            double val = 0;
            SetColor(11);
            std::cout << "  > " << prompt << ": ";
            SetColor(15);
            std::cin >> val;
            return val;
        };

        r.total = getDouble("Enter Total");
        r.expense = getDouble("Enter Expense");
        r.swipe = getDouble("Enter Swipe");
        r.qr = getDouble("Enter QR");
        r.cash = getDouble("Enter Cash");
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        records.push_back(r);
        PrintSuccess("Record added for " + r.name + ".");

        SetColor(14);
        std::cout << "\n  > Add another person? (y/n): ";
        SetColor(15);
        std::string choice;
        std::getline(std::cin, choice);
        if (choice != "y" && choice != "Y") keepAdding = false;
    }
    Pause();
}

void printA4(const std::vector<Record>& records, const std::string& appDir) {
    ClearScreen();
    PrintHeader();
    if (records.empty()) {
        PrintError("No data to print. Add data first.");
        Pause();
        return;
    }

    std::string htmlFile = appDir + "\\print_report.html";
    std::ofstream out(htmlFile);

    out << R"(<!DOCTYPE html>
<html><head><title>Expense Report</title>
<style>
    @page { size: A4; margin: 1.5cm; }
    body { font-family: Arial, sans-serif; text-align: center; color: #333; }
    h2 { margin-bottom: 20px; }
    table { margin: 0 auto; border-collapse: collapse; width: 100%; max-width: 800px; }
    th, td { border: 1px solid #000; padding: 10px; text-align: center; }
    th { background-color: #f4f4f4; font-weight: bold; }
    .footer { margin-top: 30px; font-size: 12px; color: #666; }
</style></head><body>
<h2>Daily Expense & Collection Report</h2>
<table>
<tr><th>Date</th><th>Name</th><th>Total</th><th>Expense</th><th>Swipe</th><th>QR</th><th>Cash</th></tr>
)";

    for (const auto& r : records) {
        out << "<tr>"
            << "<td>" << r.date << "</td>"
            << "<td>" << r.name << "</td>"
            << "<td>" << std::fixed << std::setprecision(2) << r.total << "</td>"
            << "<td>" << r.expense << "</td>"
            << "<td>" << r.swipe << "</td>"
            << "<td>" << r.qr << "</td>"
            << "<td>" << r.cash << "</td>"
            << "</tr>\n";
    }

    out << R"(</table>
<div class="footer">Generated automatically by Expense Manager</div>
<script>window.onload = function() { window.print(); }</script>
</body></html>)";
    out.close();

    PrintSuccess("A4 Print file generated at: " + htmlFile);
    PrintInfo("Opening print preview in your default browser...");
    system(("start \"\" \"" + htmlFile + "\"").c_str());
    Pause();
}

void backupData(const std::vector<Record>& records, const std::string& appDir) {
    ClearScreen();
    PrintHeader();
    if (records.empty()) {
        PrintError("No data to backup.");
        Pause();
        return;
    }

    std::string backupFile = appDir + "\\data_backup.csv";
    std::ofstream out(backupFile);
    out << "Date,Name,Total,Expense,Swipe,QR,Cash\n";
    
    for (const auto& r : records) {
        out << r.date << "," << r.name << "," 
            << r.total << "," << r.expense << "," 
            << r.swipe << "," << r.qr << "," << r.cash << "\n";
    }
    out.close();
    
    PrintSuccess("Backup saved to: " + backupFile);
    Pause();
}

void viewData(const std::vector<Record>& records) {
    ClearScreen();
    PrintHeader();
    SetColor(14);
    std::cout << "\n  --- [V] VIEW DATA ---\n";
    SetColor(15);
    
    std::cout << "\n  [1] Today (" << getToday() << ")\n";
    std::cout << "  [2] Yesterday (" << getYesterday() << ")\n";
    std::cout << "  [3] Specific Date (YYYY-MM-DD)\n";
    
    SetColor(11);
    std::cout << "\n  >>> Choose option: ";
    SetColor(15);
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string targetDate;
    if (choice == 1) targetDate = getToday();
    else if (choice == 2) targetDate = getYesterday();
    else {
        SetColor(11);
        std::cout << "  > Enter Date (YYYY-MM-DD): ";
        SetColor(15);
        std::getline(std::cin, targetDate);
    }

    ClearScreen();
    PrintHeader();
    SetColor(14);
    std::cout << "\n  --- Records for " << targetDate << " ---\n";
    SetColor(8);
    std::cout << "  ---------------------------------------------------------------------\n";
    SetColor(11);
    std::cout << std::left << std::setw(12) << "  Date" 
              << std::setw(15) << "Name" 
              << std::setw(10) << "Total" 
              << std::setw(10) << "Exp" 
              << std::setw(10) << "Swipe" 
              << std::setw(10) << "QR" 
              << std::setw(10) << "Cash" << "\n";
    SetColor(8);
    std::cout << "  ---------------------------------------------------------------------\n";
    SetColor(15);

    int count = 0;
    for (const auto& r : records) {
        if (r.date == targetDate) {
            std::cout << std::left << std::setw(12) << "  " + r.date 
                      << std::setw(15) << r.name 
                      << std::setw(10) << r.total 
                      << std::setw(10) << r.expense 
                      << std::setw(10) << r.swipe 
                      << std::setw(10) << r.qr 
                      << std::setw(10) << r.cash << "\n";
            count++;
        }
    }
    if (count == 0) {
        PrintError("No records found for this date.");
    } else {
        PrintSuccess("Found " + std::to_string(count) + " record(s).");
    }
    Pause();
}

int main() {
    InitConsole();
    
    std::string appDir = getAppDir();
    std::vector<Record> records;

    while (true) {
        PrintMenu();

        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1) addRecords(records);
        else if (choice == 2) printA4(records, appDir);
        else if (choice == 3) backupData(records, appDir);
        else if (choice == 4) viewData(records);
        else if (choice == 5) {
            ClearScreen();
            PrintHeader();
            SetColor(14);
            std::cout << "\n  Exiting program. Goodbye!\n\n";
            SetColor(15);
            break;
        }
        else {
            PrintError("Invalid choice. Please try again.");
            Pause();
        }
    }
    
    // Reset console colors to default before exiting
    system("color 07");
    return 0;
}
