#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <range/v3/all.hpp>
#include <tuple>
#include <map>
#include <algorithm>
#include <numeric>

auto isChapterStart = [](const std::string &line)
{
    return line.find("CHAPTER") != std::string::npos;
};

auto addChapter = [](const std::vector<std::string> chapters, const std::string currentChapter){
    if (!currentChapter.empty())
    {
        auto updatedChapters = chapters;
        updatedChapters.push_back(std::move(currentChapter));
        return std::make_tuple(updatedChapters, std::string());
    }
    else{
        return std::make_tuple(chapters, currentChapter);
    }
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

auto readFileIntoChapters = [](const std::string filePath)
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

//for 3) Tokenize text
auto tokenizeChapter = [](const std::string& chapter) -> std::vector<std::string>
{
    std::vector<std::string> words;

    // Use a stringstream to split the chapter into words
    std::istringstream iss(chapter);
    ranges::copy(ranges::istream<std::string>(iss), ranges::back_inserter(words));

    // Remove punctuation and convert to lowercase using ranges::transform and lambdas
    ranges::transform(words, words.begin(), [](std::string& word)
    {
        auto isPunctuation = [](char c) { return std::ispunct(c); };
        word.erase(ranges::remove_if(word, isPunctuation), word.end());
        ranges::transform(word, word.begin(), ::tolower);
        return word;
    });

    return words;
};

//for 3) Tokenize text
auto createWordList = [](const std::vector<std::string>& chaptersContent) -> std::vector<std::string>
{
    std::vector<std::string> wordList;

    // Iterate through each chapter using ranges::for_each
    ranges::for_each(chaptersContent, [&](const std::string& chapter)
    {
        // Tokenize the current chapter and add words to the word list
        auto chapterWords = tokenizeChapter(chapter);
        wordList.insert(wordList.end(), chapterWords.begin(), chapterWords.end());
    });

    return wordList;
};

// for 4) Function to filter words from a list based on another list
/*
auto filterWords = [](const std::vector<std::string>& wordsToFilter, const std::vector<std::string>& filterList) -> std::vector<std::string>
{
    std::vector<std::string> filteredWords;

    // Use ranges::copy_if to filter words based on the filterList
    ranges::copy_if(wordsToFilter, ranges::back_inserter(filteredWords), [&](const std::string& word)
    {
        return ranges::find(filterList, word) == filterList.end();
    });

    return filteredWords;
};*/

// for 5) Function to count word occurrences in a list
// Map function: takes a word and returns a key-value pair (word, 1)
auto MapOccurences = [](const std::string& word) {
    return std::make_pair(word, 1);
};

// Reduce function: takes a key-value pair (word, [1, 1, ...]) and sums the values
auto ReduceOccurences = [](const std::map<std::string, std::vector<int>>& groupedData) {
    std::map<std::string, int> result;
    ranges::for_each(groupedData, [&result](const auto& pair) {
        result[pair.first] = std::accumulate(pair.second.begin(), pair.second.end(), 0);
    });
    return result;
};

auto WordCountMapReduce = [](const std::vector<std::string>& wordList) {
    // Map step
    std::vector<std::pair<std::string, int>> mappedData;
    std::transform(wordList.begin(), wordList.end(), std::back_inserter(mappedData), MapOccurences);

    // Group the mapped data by key
    std::map<std::string, std::vector<int>> groupedData;
    ranges::for_each(mappedData, [&](const auto& pair) {
        groupedData[pair.first].push_back(pair.second);
    });

    // Reduce step
    auto reducedData = ReduceOccurences(groupedData);

    return reducedData;
};


// Beispiel für die Verwendung der Funktion
int main()
{
    auto chapters = readFileIntoChapters("./txt-files/war_and_peace.txt");
    std::cout << std::get<1>(chapters).size() << " Kapitel" << std::endl;
    const std::vector<std::string> chaptersContent = std::get<1>(chapters);

    std::cout << chaptersContent[0] << std::endl;

    // Create the word list by processing each chapter
    std::vector<std::string> wordList = createWordList(chaptersContent);

/* 
    // Print the complete tokenized word list
    std::cout << "Complete Tokenized Word List:\n";
    for (const auto& word : wordList)
    {
        std::cout << word << std::endl;
    }
    std::cout << "-----------------------------\n";


    //Print the word occurrences
    std::cout << "Word Occurrences:\n";
    for (const auto& entry : finalResult)
    {
        std::cout << entry.first << ": " << entry.second << " occurrences\n";
    }*/

    auto reducedData = WordCountMapReduce(wordList);

    // Print the result
    for (const auto& entry : reducedData) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }

    return 0;
}