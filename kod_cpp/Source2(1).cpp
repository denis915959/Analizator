#include <iostream> //библиотеки
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <algorithm>

using namespace std;

int number_cycles = 0;
int b;
int b1;
int coord1;

double a_min0;

double a_max0;

double znachen;
double znachen1;
double znachen2;
double plosh;
double volumec = 0.000008305;
double V_S;

double assimil1;
double assimil2;
double assimil3;

double razn01;
double razn02;
double razn03;
std::vector<double> razn1;
std::vector<double> razn2;
std::vector<double> razn3;
std::vector<double> razn0;
std::vector<double> listCO21;
std::vector<double> listCO22;
std::vector<double> listCO23;
//std::vector<double> temp1;
//std::vector<double> temp2;
//std::vector<double> temp3;
//std::vector<double> time1;
std::vector<double> b_begin;
std::vector<double> b_end;
std::vector<std::string> segments;

void assim() {
    razn1.clear();
    razn2.clear();
    razn3.clear();
    razn01 = 0;
    razn02 = 0;
    razn03 = 0;
    int cyclass = listCO21.size();
    for (int i4 = 3; i4 < cyclass; i4++) {
        if ((i4 + 30) < cyclass) {
            razn01 = listCO21[i4 + 30] - listCO21[i4];
            if (abs(razn01) <= 40) {
                razn1.push_back(razn01);
            }
            razn02 = listCO22[i4 + 30] - listCO22[i4];
            if (abs(razn02) <= 40) {
                razn2.push_back(razn02);
            }
            razn03 = listCO23[i4 + 30] - listCO23[i4];
            if (abs(razn03) <= 40) {
                razn3.push_back(razn03);
            }
        }
    }
}
void min_max_val() {
    b1 = razn0.size();
    for (int i7 = 0; i7 < b1; i7++) {
        if (i7 == 0) {
            a_min0 = razn0[i7];
        }
        else {
            if (a_min0 > razn0[i7]) {
                a_min0 = razn0[i7];
            }
        }
    }

    for (int i8 = 0; i8 < b1; i8++) {
        if (i8 == 0) {
            a_max0 = razn0[i8];
        }
        else {
            if (a_max0 < razn0[i8]) {
                a_max0 = razn0[i8];
            }
        }
    }

}
void val() {
    for (int i5 = 0; i5 < 3; i5++) {
        int rvm;
        razn0.clear();
        rvm=razn1.size();
        if (rvm != 0) {
            if (i5 == 0) {
                razn0.assign(razn1.begin(), razn1.end());
                min_max_val();
                assimil1 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
            if (i5 == 1) {
                razn0.assign(razn2.begin(), razn2.end());
                min_max_val();
                assimil2 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
            if (i5 == 2) {
                razn0.assign(razn3.begin(), razn3.end());
                min_max_val();
                assimil3 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    std::cout << "Введите значение площади листа в м^2    ";
    std::cin >> plosh;
    std::string filename1;
    std::cout << "       Введите название файла (например, data.txt или полный путь к файлу): ";
    std::cin >> filename1;

    V_S = volumec / plosh;
    std::string line;
    std::ifstream inputFile(filename1);

    
    //std::vector<std::string> elements;*/
    while (std::getline(inputFile, line)) { // обход по всем строкам файла
        std::stringstream elem(line);
        std::string segment;


        while (std::getline(elem, segment, ';')) { // Сначала по запятой
             // Если сегмент содержит точку с запятой, нужно разбить его дальше
            std::stringstream sub_elem(segment);
            std::string sub_segment;
            while (std::getline(sub_elem, sub_segment, ' ')) {
                if (!sub_segment.empty()) { // Пропускаем пустые сегменты
                    segments.push_back(sub_segment);
                }
            }
        }
    }
    b = segments.size(); // определить размер массива скачанных из файла элементов

    std::string searchText = "CO2_1";
    for (int i1 = 0; i1 < b; i1++) { // определение координат записей и количества записей в файле
        if (i1 == 0) {
            if (segments[i1] == searchText) {
                int i11;
                i11 = i1 + 11;
                b_begin.push_back(i11);
                number_cycles++;
            }
        }
        else {
            if (segments[i1] == searchText) {
                int i12;
                int i13;
                i12 = i1 + 11;
                i13 = i1 - 1;
                b_begin.push_back(i12);
                b_end.push_back(i13);
                number_cycles++;
            }
        }

    }
    int bv;
    bv = b - 1;
    b_end.push_back(bv);

    ofstream fileresults;
    fileresults.open("results.txt");
    fileresults << "Запись" << ";" << "датчик" << "; " << "мкмоль м-2 с-1" << "; " << "датчик" << "; " << "мкмоль м-2 с-1" << "; " << "медиана" << "; " << "мкмоль м-2 с-1" << "; " << '\n';

    for (int i2 = 0; i2 < number_cycles; i2++) {
        int loccycl;
        loccycl = b_end[i2] - b_begin[i2] + 1;
        for (int i3 = 0; i3 < loccycl; i3++) {
            if (i3 % 11 == 0) {
                coord1 = b_begin[i2] + i3;
                znachen = std::stof(segments[coord1]);
                znachen1 = std::stof(segments[coord1 + 1]);
                znachen2 = std::stof(segments[coord1 + 2]);
                listCO21.push_back(znachen);
                listCO22.push_back(znachen1);
                listCO23.push_back(znachen2);
                znachen = 0;
                znachen1 = 0;
                znachen2 = 0;
            }
        }

        assim();
        val();
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "Запись " << i2+1 << ": 1 датчик: " << assimil1 << ";  " << "2 датчик: " << assimil2 << ";  " << "медиана: " << assimil3 << std::endl;
        fileresults << "Запись " << i2 + 1 << ";" << "1 датчик" << ";" << assimil1 << ";" << "2 датчик" << ";" << assimil2 << ";" << "медиана" << ";" << assimil3 << ";" << '\n';
        listCO21.clear();
        listCO22.clear();
        listCO23.clear();
        razn1.clear();
        razn2.clear();
        razn3.clear();
        assimil1 = 0;
        assimil2 = 0;
        assimil3 = 0;
    }

    inputFile.close();
    fileresults.close();
    
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << "     Результаты записаны в results.txt" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

    

    char input;
    std::cout << "Введите '1' для окончания" << std::endl;
    std::cin >> input;
    if (input == '1') {
        std::cout << "Конец" << std::endl;
        return 0; 
    }
   
}