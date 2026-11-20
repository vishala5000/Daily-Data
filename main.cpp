#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace fs = std::filesystem;

struct Record {
    std::string date;
    std::string name;
    double total;
    double expense;
    double swipe;
    double qr;
    double cash;
};

// Get formatted date (YYYY-MM-DD)
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

// Get the directory where the application is running
fs::path getAppDir(const char* argv0) {
    fs::path p = fs::path(argv0).parent_path();
    return p.empty() ? fs::current_path() : p;
}

void addRecords(std::vector<Record>& records) {
    std::cout << "\n--- ADD NEW DATA ---\n";
    bool keepAdding = true;
    while (keepAdding) {
        Record r;
        std::cout << "Enter Date (YYYY-MM-DD) [Press Enter for Today]: ";
        std::getline(std::cin, r.date);
        if (r.date.empty()) r.date = getToday();

        std::cout << "Enter Name: ";
        std::getline(std::cin, r.name);

        std::cout << "Enter Total: ";
        std::cin >> r.total;
        std::cout << "Enter Expense: ";
        std::cin >> r.expense;
        std::cout << "Enter Swipe: ";
        std::cin >> r.swipe;
        std::cout << "Enter QR: ";
        std::cin >> r.qr;
        std::cout << "Enter Cash: ";
        std::cin >> r.cash;
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer

        records.push_back(r);
        std::cout << "\n[Success] Record added for " << r.name << ".\n";

        std::cout << "Add another person? (y/n): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice != "y" && choice != "Y") keepAdding = false;
    }
}

void printA4(const std::vector<Record>& records, const fs::path& appDir) {
    if (records.empty()) {
        std::cout << "\n[Error] No data to print. Add data first.\n";
        return;
    }

    fs::path htmlFile = appDir / "print_report.html";
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

    std::cout << "\n[Success] A4 Print file generated at: " << htmlFile.string() << "\n";
    std::cout << "Opening print preview... (If it doesn't open, please open the HTML file manually).\n";
    
    // Cross-platform open command
    #ifdef _WIN32
        system(("start " + htmlFile.string()).c_str());
    #elif __APPLE__
        system(("open " + htmlFile.string()).c_str());
    #else
        system(("xdg-open " + htmlFile.string()).c_str());
    #endif
}

void backupData(const std::vector<Record>& records, const fs::path& appDir) {
    if (records.empty()) {
        std::cout << "\n[Error] No data to backup.\n";
        return;
    }

    fs::path backupFile = appDir / "data_backup.csv";
    std::ofstream out(backupFile);
    out << "Date,Name,Total,Expense,Swipe,QR,Cash\n";
    
    for (const auto& r : records) {
        out << r.date << "," << r.name << "," 
            << r.total << "," << r.expense << "," 
            << r.swipe << "," << r.qr << "," << r.cash << "\n";
    }
    out.close();
    std::cout << "\n[Success] Backup saved to: " << backupFile.string() << "\n";
}

void viewData(const std::vector<Record>& records) {
    std::cout << "\n--- VIEW DATA ---\n";
    std::cout << "1. Today (" << getToday() << ")\n";
    std::cout << "2. Yesterday (" << getYesterday() << ")\n";
    std::cout << "3. Specific Date (YYYY-MM-DD)\n";
    std::cout << "Choose option: ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string targetDate;
    if (choice == 1) targetDate = getToday();
    else if (choice == 2) targetDate = getYesterday();
    else {
        std::cout << "Enter Date (YYYY-MM-DD): ";
        std::getline(std::cin, targetDate);
    }

    std::cout << "\n--- Records for " << targetDate << " ---\n";
    std::cout << std::left << std::setw(12) << "Date" 
              << std::setw(15) << "Name" 
              << std::setw(10) << "Total" 
              << std::setw(10) << "Exp" 
              << std::setw(10) << "Swipe" 
              << std::setw(10) << "QR" 
              << std::setw(10) << "Cash" << "\n";
    std::cout << std::string(77, '-') << "\n";

    int count = 0;
    for (const auto& r : records) {
        if (r.date == targetDate) {
            std::cout << std::left << std::setw(12) << r.date 
                      << std::setw(15) << r.name 
                      << std::setw(10) << r.total 
                      << std::setw(10) << r.expense 
                      << std::setw(10) << r.swipe 
                      << std::setw(10) << r.qr 
                      << std::setw(10) << r.cash << "\n";
            count++;
        }
    }
    if (count == 0) std::cout << "No records found for this date.\n";
}

int main(int argc, char* argv[]) {
    fs::path appDir = getAppDir(argv[0]);
    std::vector<Record> records;

    while (true) {
        std::cout << "\n================ MAIN MENU ================\n";
        std::cout << "1. Add Data (Endless Entry)\n";
        std::cout << "2. Print Data (A4 Size Perfectly Aligned)\n";
        std::cout << "3. Backup All Data\n";
        std::cout << "4. View Data (Date-wise)\n";
        std::cout << "5. Exit\n";
        std::cout << "===========================================\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 1) addRecords(records);
        else if (choice == 2) printA4(records, appDir);
        else if (choice == 3) backupData(records, appDir);
        else if (choice == 4) viewData(records);
        else if (choice == 5) {
            std::cout << "Exiting program. Goodbye!\n";
            break;
        }
        else {
            std::cout << "[Error] Invalid choice. Please try again.\n";
        }
    }
    return 0;
}
