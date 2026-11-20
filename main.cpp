name: Build Windows App (Win 7/8/10/11)

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]

jobs:
  build:
    name: Build Windows Executable
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v4

    - name: Compile C++ Code (Statically Linked)
      shell: bash
      run: |
        # -static ensures no external DLLs are required (Crucial for Win 7/8)
        # -std=c++17 enables modern C++ features
        g++ -static -std=c++17 main.cpp -o ExpenseApp.exe

    - name: Upload Executable Artifact
      uses: actions/upload-artifact@v4
      with:
        name: ExpenseApp-Windows
        path: ExpenseApp.exe
