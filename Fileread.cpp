#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <range/v3/all.hpp>
#include <tuple>

auto isChapterStart = [](const std::string &line)
{
    return line.find("CHAPTER") != std::string::npos;
};

auto addChapter = [](std::vector<std::string> chapters, std::string currentChapter)
{
    if (!currentChapter.empty())
    {
        chapters.push_back(std::move(currentChapter));
        currentChapter.clear();
    }
    auto result = std::make_tuple(chapters, currentChapter);
    return result;
};

auto getLines = [](const std::string filePath)
{
    std::ifstream file(filePath);
    std::vector<std::string> lines;

    if (file.is_open())
    {
        std::string line;
        while (getline(file, line))
        {
            lines.push_back(line);
        }
    }
    else
    {
        return std::make_tuple(false, lines);
    }

    return std::make_tuple(true, lines);
};

auto readFileIntoChapters = [](std::string filePath)
{
    std::vector<std::string> chapters;
    std::string currentChapter;
    bool foundFirstChapter = false;

    auto getLinesResult = getLines(filePath);
    auto lines = std::get<1>(getLinesResult);

    if (std::get<0>(getLinesResult) == true)
    {
        ranges::for_each(lines, [&](const std::string &line)
                         {
            if (isChapterStart(line)) {
                if (foundFirstChapter) {
                    auto addChapterResult = addChapter(chapters, currentChapter);
                    chapters = std::get<0>(addChapterResult);
                    currentChapter = std::get<1>(addChapterResult);
                }
                foundFirstChapter = true;
            } else if (foundFirstChapter) {
                currentChapter += line + '\n';
            } });
        addChapter(chapters, currentChapter); // Don't forget to add the last chapter
    }
    else
    {
        return std::make_tuple(false, chapters);
    }

    return std::make_tuple(true, chapters);
};

// Beispiel für die Verwendung der Funktion
int main()
{
    auto chapters = readFileIntoChapters("./txt-files/war_and_peace.txt");
    std::cout << std::get<1>(chapters).size() << " Kapitel" << std::endl;
    return 0;
}