#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <range/v3/all.hpp>
#include <tuple>

// Funktion zum Überprüfen, ob eine Zeile ein Kapitelanfang ist
auto isChapterStart = [](const std::string& line) {
    return line.find("CHAPTER") != std::string::npos;
};

// Funktion zum Hinzufügen eines Kapitels zu einem Vektor von Kapiteln
auto addChapter = [](std::vector<std::string> chapters, std::string currentChapter) {
    if (!currentChapter.empty()) {
        chapters.push_back(std::move(currentChapter));
        currentChapter.clear();
    }
    return make_tuple(chapters, currentChapter);
};

// Funktion, um den Dateiinhalt in Kapitel zu konvertieren
auto readFileIntoChapters = [](const std::string& filePath) {
    std::ifstream file(filePath);
    std::vector<std::string> chapters;
    std::string currentChapter;
    bool foundFirstChapter = false;

    if (file.is_open()) {
        ranges::for_each(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
            [&](char ch) {
                if (ch == '\n') {
                    if (isChapterStart(currentChapter)) {
                        if (foundFirstChapter) {
                            auto addchapter_tuple = addChapter(chapters, currentChapter);
                            chapters = std::get<0>(addchapter_tuple);
                            currentChapter = std::get<1>(addchapter_tuple);
                        }
                        foundFirstChapter = true;
                    }
                    currentChapter.clear();
                } else {
                    currentChapter += ch;
                }
            });
        // Fügen Sie das letzte Kapitel hinzu, falls vorhanden
        addChapter(chapters, currentChapter);
    } else {
        std::cerr << "Could not open file: " << filePath << std::endl;
    }

    return chapters;
};

int main() {
    const std::string filePath = "./txt-files/war_and_peace.txt";
    auto chapters = readFileIntoChapters(filePath);
    
    // Output chapters for verification
    for (const auto& chapter : chapters) {
        std::cout << chapter << "\n---\n";
    }

    return 0;
}
