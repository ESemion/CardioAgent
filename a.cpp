#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ofstream file("C:/Users/Vladislave/Desktop/CardioAgent/CardioAgent/CardioAgent/results/results.txt");
    
    if (!file.is_open()) {
        cout << "Ошибка: не удалось открыть файл!" << endl;
        return 1;
    }
    
    file << "Программа успешно завершилась";
    file.close();
    
    cout << "Файл сохранен!" << endl;
    return 0;
}